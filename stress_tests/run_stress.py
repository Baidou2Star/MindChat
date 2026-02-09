#!/usr/bin/env python3
import argparse
import asyncio
import configparser
import json
import math
import os
import resource
import shlex
import struct
import subprocess
import sys
import time
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple


MSG_CHAT_LOGIN = 1005
MSG_CHAT_LOGIN_RSP = 1006
ID_HEART_BEAT_REQ = 1023
ID_HEARTBEAT_RSP = 1024


def normalize_peer_host(peer_host: str, fallback_host: str) -> str:
    host = (peer_host or "").strip()
    if not host:
        return fallback_host
    if host in {"127.0.0.1", "localhost", "0.0.0.0", "::1", "::"}:
        return fallback_host
    return host


def now_iso() -> str:
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())


def ts_id() -> str:
    return time.strftime("%Y%m%d_%H%M%S", time.localtime())


def safe_json_loads(text: str) -> Dict:
    try:
        data = json.loads(text)
        if isinstance(data, dict):
            return data
        return {}
    except json.JSONDecodeError:
        return {}


def percentile(values: Sequence[float], pct: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    index = max(0, min(len(ordered) - 1, math.ceil((pct / 100.0) * len(ordered)) - 1))
    return float(ordered[index])


def latency_stats(latencies_ms: Sequence[float]) -> Dict[str, float]:
    if not latencies_ms:
        return {
            "count": 0,
            "min": 0.0,
            "avg": 0.0,
            "p50": 0.0,
            "p95": 0.0,
            "p99": 0.0,
            "max": 0.0,
        }
    ordered = sorted(latencies_ms)
    total = sum(ordered)
    return {
        "count": len(ordered),
        "min": float(ordered[0]),
        "avg": float(total / len(ordered)),
        "p50": percentile(ordered, 50),
        "p95": percentile(ordered, 95),
        "p99": percentile(ordered, 99),
        "max": float(ordered[-1]),
    }


def chunked(items: Sequence, size: int):
    for index in range(0, len(items), size):
        yield items[index:index + size]


def ini_get(config_path: Path, section: str, key: str, default: Optional[str] = None) -> Optional[str]:
    parser = configparser.ConfigParser()
    if not config_path.exists():
        return default
    parser.read(config_path, encoding="utf-8")
    if not parser.has_section(section):
        return default
    if parser.has_option(section, key):
        return parser.get(section, key).strip()
    return default


def parse_http_response(raw: bytes) -> Tuple[int, str]:
    head_part, sep, body_part = raw.partition(b"\r\n\r\n")
    if not sep:
        return 0, ""
    head_lines = head_part.split(b"\r\n")
    if not head_lines:
        return 0, body_part.decode("utf-8", errors="replace")
    status_line = head_lines[0].decode("utf-8", errors="replace")
    status_code = 0
    parts = status_line.split()
    if len(parts) >= 2 and parts[1].isdigit():
        status_code = int(parts[1])

    headers: Dict[str, str] = {}
    for line in head_lines[1:]:
        text_line = line.decode("utf-8", errors="replace")
        if ":" not in text_line:
            continue
        header_key, header_val = text_line.split(":", 1)
        headers[header_key.strip().lower()] = header_val.strip()

    transfer_encoding = headers.get("transfer-encoding", "").lower()
    if "chunked" in transfer_encoding:
        body_part = decode_chunked(body_part)
    return status_code, body_part.decode("utf-8", errors="replace")


def decode_chunked(body: bytes) -> bytes:
    data = body
    result = bytearray()
    while data:
        line, sep, remain = data.partition(b"\r\n")
        if not sep:
            break
        try:
            chunk_size = int(line.split(b";", 1)[0], 16)
        except ValueError:
            break
        if chunk_size == 0:
            break
        chunk = remain[:chunk_size]
        result.extend(chunk)
        data = remain[chunk_size + 2:]
    return bytes(result)


def pack_frame(msg_id: int, payload_text: str) -> bytes:
    payload = payload_text.encode("utf-8")
    if len(payload) > 65535:
        raise ValueError(f"payload too large: {len(payload)}")
    return struct.pack("!HH", msg_id, len(payload)) + payload


async def read_frame(reader: asyncio.StreamReader, timeout_sec: float) -> Tuple[int, str]:
    header = await asyncio.wait_for(reader.readexactly(4), timeout=timeout_sec)
    msg_id, body_len = struct.unpack("!HH", header)
    body = await asyncio.wait_for(reader.readexactly(body_len), timeout=timeout_sec)
    return msg_id, body.decode("utf-8", errors="replace")


class RunLogger:
    def __init__(self, mode: str, log_root: Path):
        self.mode = mode
        self.run_dir = log_root / f"{ts_id()}_{mode}"
        self.run_dir.mkdir(parents=True, exist_ok=True)
        self.events_file = self.run_dir / "events.log"
        self.summary_file = self.run_dir / "summary.json"

    def log(self, level: str, message: str, **fields):
        payload = {
            "ts": now_iso(),
            "level": level.upper(),
            "message": message,
        }
        if fields:
            payload["fields"] = fields
        line = json.dumps(payload, ensure_ascii=False)
        print(line)
        with self.events_file.open("a", encoding="utf-8") as handle:
            handle.write(line + "\n")

    def write_summary(self, summary: Dict):
        with self.summary_file.open("w", encoding="utf-8") as handle:
            json.dump(summary, handle, ensure_ascii=False, indent=2)


@dataclass
class ChatClient:
    account_index: int
    email: str
    password: str
    uid: int
    token: str
    chat_host: str
    chat_port: int
    reader: asyncio.StreamReader
    writer: asyncio.StreamWriter
    alive: bool = True


class StressHarness:
    def __init__(self, args: argparse.Namespace, logger: RunLogger):
        self.args = args
        self.logger = logger
        self.clients: List[ChatClient] = []
        self.failure_reasons: Counter = Counter()
        self.next_user_index = args.user_start

    @property
    def alive_clients(self) -> List[ChatClient]:
        return [client for client in self.clients if client.alive]

    def _record_failure(self, reason: str):
        self.failure_reasons[reason] += 1

    def _email_of(self, account_index: int) -> str:
        return f"{self.args.user_prefix}{account_index}@{self.args.email_domain}"

    async def _http_post_json(self, host: str, port: int, path: str, payload: Dict) -> Tuple[int, str]:
        body = json.dumps(payload, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
        request_lines = [
            f"POST {path} HTTP/1.1",
            f"Host: {host}:{port}",
            "Content-Type: application/json",
            f"Content-Length: {len(body)}",
            "Connection: close",
            "",
            "",
        ]
        request_bytes = "\r\n".join(request_lines).encode("utf-8") + body

        reader: Optional[asyncio.StreamReader] = None
        writer: Optional[asyncio.StreamWriter] = None
        try:
            reader, writer = await asyncio.wait_for(
                asyncio.open_connection(host, port),
                timeout=self.args.connect_timeout,
            )
            writer.write(request_bytes)
            await asyncio.wait_for(writer.drain(), timeout=self.args.request_timeout)
            response_raw = await asyncio.wait_for(reader.read(-1), timeout=self.args.request_timeout)
            return parse_http_response(response_raw)
        finally:
            if writer is not None:
                writer.close()
                try:
                    await writer.wait_closed()
                except Exception:
                    pass

    @staticmethod
    def _exc_reason(prefix: str, exc: Exception) -> str:
        if isinstance(exc, asyncio.TimeoutError):
            return f"{prefix}_timeout"
        if isinstance(exc, ConnectionRefusedError):
            return f"{prefix}_refused"
        if isinstance(exc, ConnectionResetError):
            return f"{prefix}_reset"
        if isinstance(exc, OSError):
            if exc.errno is not None:
                return f"{prefix}_oserror_{exc.errno}"
            return f"{prefix}_oserror"
        return f"{prefix}_{exc.__class__.__name__.lower()}"

    async def connect_one(self, account_index: int) -> Optional[ChatClient]:
        email = self._email_of(account_index)
        password = self.args.password
        gate_retry = max(0, int(self.args.gate_login_retries))
        retry_backoff_sec = max(0.0, float(self.args.retry_backoff_ms) / 1000.0)

        uid = 0
        token = ""
        chat_host = self.args.default_chat_host
        chat_port = 0
        for gate_attempt in range(gate_retry + 1):
            try:
                status_code, body = await self._http_post_json(
                    self.args.gate_host,
                    self.args.gate_port,
                    "/user_login",
                    {"email": email, "passwd": password},
                )
            except Exception as exc:
                reason = self._exc_reason("gate_login", exc)
                if gate_attempt < gate_retry:
                    await asyncio.sleep(retry_backoff_sec)
                    continue
                self._record_failure(reason)
                return None

            if status_code != 200:
                self._record_failure(f"gate_http_{status_code}")
                return None

            login_obj = safe_json_loads(body)
            login_error = int(login_obj.get("error", -1))
            if login_error != 0:
                if login_error == 1002 and gate_attempt < gate_retry:
                    await asyncio.sleep(retry_backoff_sec)
                    continue
                self._record_failure(f"gate_login_error_{login_error}")
                return None

            uid = int(login_obj.get("uid", 0))
            token = str(login_obj.get("token", ""))
            chat_host = normalize_peer_host(
                str(login_obj.get("chathost", "")),
                self.args.default_chat_host,
            )
            chat_port_raw = str(login_obj.get("chatport", "")).strip()

            if uid <= 0 or not token:
                self._record_failure("gate_login_missing_uid_or_token")
                return None

            if not chat_port_raw.isdigit():
                self._record_failure("gate_login_invalid_chat_port")
                return None

            chat_port = int(chat_port_raw)
            break

        writer: Optional[asyncio.StreamWriter] = None
        chat_retry = max(0, int(self.args.chat_connect_retries))
        for chat_attempt in range(chat_retry + 1):
            try:
                reader, writer = await asyncio.wait_for(
                    asyncio.open_connection(chat_host, chat_port),
                    timeout=self.args.connect_timeout,
                )

                login_payload = json.dumps({"uid": uid, "token": token}, separators=(",", ":"), ensure_ascii=False)
                writer.write(pack_frame(MSG_CHAT_LOGIN, login_payload))
                await asyncio.wait_for(writer.drain(), timeout=self.args.request_timeout)

                rsp_msg_id, rsp_payload = await read_frame(reader, timeout_sec=self.args.request_timeout)
                if rsp_msg_id != MSG_CHAT_LOGIN_RSP:
                    self._record_failure(f"chat_login_rsp_msgid_{rsp_msg_id}")
                    writer.close()
                    await writer.wait_closed()
                    return None

                rsp_obj = safe_json_loads(rsp_payload)
                rsp_err = int(rsp_obj.get("error", -1))
                if rsp_err != 0:
                    self._record_failure(f"chat_login_error_{rsp_err}")
                    writer.close()
                    await writer.wait_closed()
                    return None

                return ChatClient(
                    account_index=account_index,
                    email=email,
                    password=password,
                    uid=uid,
                    token=token,
                    chat_host=chat_host,
                    chat_port=chat_port,
                    reader=reader,
                    writer=writer,
                )
            except Exception as exc:
                reason = self._exc_reason("chat_connect", exc)
                retryable = reason in {"chat_connect_oserror_99", "chat_connect_timeout", "chat_connect_reset"}
                if writer is not None:
                    writer.close()
                    try:
                        await writer.wait_closed()
                    except Exception:
                        pass
                if retryable and chat_attempt < chat_retry:
                    await asyncio.sleep(retry_backoff_sec)
                    continue
                self._record_failure(reason)
                return None
        return None

    async def add_connections_until(self, target_alive: int, extra_attempt_ratio: float) -> None:
        if len(self.alive_clients) >= target_alive:
            return

        attempt_cap = int(math.ceil(target_alive * (1.0 + extra_attempt_ratio)))
        attempt_end = self.args.user_start + attempt_cap
        no_progress_batches = 0

        while len(self.alive_clients) < target_alive and self.next_user_index < attempt_end:
            alive_before = len(self.alive_clients)
            need_count = target_alive - alive_before
            batch_size = min(self.args.connect_batch_size, need_count)
            indexes = list(range(self.next_user_index, self.next_user_index + batch_size))
            self.next_user_index += batch_size

            tasks = [asyncio.create_task(self.connect_one(account_index)) for account_index in indexes]
            batch_results = await asyncio.gather(*tasks)

            success_count = 0
            for result in batch_results:
                if result is None:
                    continue
                self.clients.append(result)
                success_count += 1

            alive_after = len(self.alive_clients)
            self.logger.log(
                "INFO",
                "batch connect done",
                batch_size=batch_size,
                success=success_count,
                alive=alive_after,
                target=target_alive,
                next_user_index=self.next_user_index,
            )

            if alive_after == alive_before:
                no_progress_batches += 1
            else:
                no_progress_batches = 0

            if no_progress_batches >= self.args.max_no_progress_batches:
                self.logger.log(
                    "WARN",
                    "stop connecting due to repeated no-progress batches",
                    no_progress_batches=no_progress_batches,
                    alive=alive_after,
                    target=target_alive,
                    failures_top=dict(self.failure_reasons.most_common(5)),
                )
                break
            interval_sec = max(0.0, float(self.args.connect_batch_interval_ms) / 1000.0)
            if interval_sec > 0:
                await asyncio.sleep(interval_sec)

    async def heartbeat_one(self, client: ChatClient) -> Tuple[bool, float, str]:
        if not client.alive:
            return False, 0.0, "client_not_alive"

        start = time.perf_counter()
        try:
            payload = json.dumps({"fromuid": client.uid}, separators=(",", ":"), ensure_ascii=False)
            client.writer.write(pack_frame(ID_HEART_BEAT_REQ, payload))
            await asyncio.wait_for(client.writer.drain(), timeout=self.args.request_timeout)

            for _ in range(3):
                msg_id, msg_payload = await read_frame(client.reader, timeout_sec=self.args.heartbeat_timeout)
                if msg_id == ID_HEARTBEAT_RSP:
                    rsp_obj = safe_json_loads(msg_payload)
                    if int(rsp_obj.get("error", -1)) != 0:
                        return False, 0.0, f"heartbeat_error_{rsp_obj.get('error', -1)}"
                    elapsed_ms = (time.perf_counter() - start) * 1000.0
                    return True, elapsed_ms, ""

            return False, 0.0, "heartbeat_rsp_not_found"
        except Exception as exc:
            return False, 0.0, self._exc_reason("heartbeat", exc)

    async def heartbeat_round(self, round_name: str) -> Dict:
        alive_snapshot = self.alive_clients
        attempts = 0
        success = 0
        latencies: List[float] = []
        failure_counter: Counter = Counter()

        for group in chunked(alive_snapshot, self.args.heartbeat_batch_size):
            tasks = [asyncio.create_task(self.heartbeat_one(client)) for client in group]
            results = await asyncio.gather(*tasks)
            for client, result in zip(group, results):
                attempts += 1
                ok, latency_ms, reason = result
                if ok:
                    success += 1
                    latencies.append(latency_ms)
                    continue
                failure_counter[reason] += 1
                self._record_failure(reason)
                client.alive = False
                client.writer.close()
                try:
                    await client.writer.wait_closed()
                except Exception:
                    pass

        failed = attempts - success
        ratio = (success / attempts) if attempts else 0.0
        stats = latency_stats(latencies)

        round_result = {
            "round_name": round_name,
            "attempts": attempts,
            "success": success,
            "failed": failed,
            "success_ratio": ratio,
            "latency_ms": stats,
            "failures": dict(failure_counter),
            "alive_after_round": len(self.alive_clients),
        }
        return round_result

    async def close_all(self):
        for client in self.clients:
            if not client.alive:
                continue
            client.alive = False
            client.writer.close()
            try:
                await client.writer.wait_closed()
            except Exception:
                pass


def merge_rounds(rounds: Sequence[Dict]) -> Dict:
    total_attempts = 0
    total_success = 0
    fail_counter: Counter = Counter()
    total_latency_count = 0
    weighted_latency_sum = 0.0
    min_latency = None
    max_latency = None
    p50_max = 0.0
    p95_max = 0.0
    p99_max = 0.0

    for item in rounds:
        total_attempts += int(item["attempts"])
        total_success += int(item["success"])
        for key, val in item.get("failures", {}).items():
            fail_counter[key] += int(val)

        stats = item.get("latency_ms", {})
        count = int(stats.get("count", 0))
        if count > 0:
            avg = float(stats.get("avg", 0.0))
            weighted_latency_sum += avg * count
            total_latency_count += count

            cur_min = float(stats.get("min", 0.0))
            cur_max = float(stats.get("max", 0.0))
            min_latency = cur_min if min_latency is None else min(min_latency, cur_min)
            max_latency = cur_max if max_latency is None else max(max_latency, cur_max)

            p50_max = max(p50_max, float(stats.get("p50", 0.0)))
            p95_max = max(p95_max, float(stats.get("p95", 0.0)))
            p99_max = max(p99_max, float(stats.get("p99", 0.0)))

    success_ratio = (total_success / total_attempts) if total_attempts else 0.0
    if total_latency_count > 0:
        merged_latency = {
            "count": total_latency_count,
            "min": float(min_latency if min_latency is not None else 0.0),
            "avg": float(weighted_latency_sum / total_latency_count),
            "p50": p50_max,
            "p95": p95_max,
            "p99": p99_max,
            "max": float(max_latency if max_latency is not None else 0.0),
        }
    else:
        merged_latency = latency_stats([])

    return {
        "attempts": total_attempts,
        "success": total_success,
        "failed": total_attempts - total_success,
        "success_ratio": success_ratio,
        "latency_ms": merged_latency,
        "failures": dict(fail_counter),
    }


def check_nofile(logger: RunLogger, target_connections: int):
    soft_limit, hard_limit = resource.getrlimit(resource.RLIMIT_NOFILE)
    suggested = target_connections * 2 + 2048
    logger.log(
        "INFO",
        "process fd limit check",
        nofile_soft=soft_limit,
        nofile_hard=hard_limit,
        suggested_min=suggested,
    )
    if soft_limit < suggested:
        logger.log(
            "WARN",
            "nofile soft limit may be too low",
            hint=f"ulimit -n {suggested}",
            nofile_soft=soft_limit,
        )


async def run_conn_limit(args: argparse.Namespace, logger: RunLogger) -> Dict:
    harness = StressHarness(args, logger)
    check_nofile(logger, args.max_connections)

    stage_target = args.start_connections
    best_stable = 0
    stop_reason = ""
    rounds_log: List[Dict] = []

    while stage_target <= args.max_connections:
        logger.log("INFO", "conn-limit stage start", stage_target=stage_target, alive=len(harness.alive_clients))
        await harness.add_connections_until(stage_target, args.extra_attempt_ratio)

        alive_count = len(harness.alive_clients)
        connect_ratio = alive_count / stage_target if stage_target else 0.0
        logger.log(
            "INFO",
            "conn-limit stage connect result",
            stage_target=stage_target,
            alive=alive_count,
            connect_ratio=round(connect_ratio, 6),
        )

        if connect_ratio < args.min_connect_ratio:
            stop_reason = f"connect_ratio_below_threshold:{connect_ratio:.4f}"
            break

        stage_rounds: List[Dict] = []
        for probe_index in range(1, args.probe_rounds + 1):
            result = await harness.heartbeat_round(f"stage_{stage_target}_probe_{probe_index}")
            stage_rounds.append(result)
            rounds_log.append(result)
            logger.log(
                "INFO",
                "conn-limit probe done",
                stage_target=stage_target,
                probe=probe_index,
                attempts=result["attempts"],
                success=result["success"],
                success_ratio=round(result["success_ratio"], 6),
                p95_ms=round(result["latency_ms"]["p95"], 3),
                alive_after_round=result["alive_after_round"],
            )

        merged = merge_rounds(stage_rounds)
        stage_ratio = float(merged["success_ratio"])
        stage_p95 = float(merged["latency_ms"]["p95"])

        if stage_ratio < args.min_heartbeat_ratio:
            stop_reason = f"heartbeat_ratio_below_threshold:{stage_ratio:.4f}"
            break
        if stage_p95 > args.latency_threshold_ms:
            stop_reason = f"heartbeat_p95_exceeds_threshold:{stage_p95:.3f}"
            break

        best_stable = len(harness.alive_clients)
        stage_target += args.step_connections
        await asyncio.sleep(args.stage_interval_sec)

    if not stop_reason:
        stop_reason = "reached_max_connections"

    summary = {
        "mode": "conn-limit",
        "stop_reason": stop_reason,
        "best_stable_connections": best_stable,
        "alive_at_end": len(harness.alive_clients),
        "failure_reasons_top": dict(harness.failure_reasons.most_common(20)),
        "rounds": rounds_log,
        "args": vars(args),
    }
    logger.write_summary(summary)
    await harness.close_all()
    return summary


async def run_stability(args: argparse.Namespace, logger: RunLogger) -> Dict:
    harness = StressHarness(args, logger)
    check_nofile(logger, args.connections)

    target = args.connections
    await harness.add_connections_until(target, args.extra_attempt_ratio)
    alive_after_connect = len(harness.alive_clients)
    connect_ratio = alive_after_connect / target if target else 0.0

    logger.log(
        "INFO",
        "stability connect complete",
        target=target,
        alive=alive_after_connect,
        connect_ratio=round(connect_ratio, 6),
    )

    round_results: List[Dict] = []
    if alive_after_connect == 0:
        summary = {
            "mode": "stability",
            "result": "failed",
            "reason": "no_alive_connections_after_connect",
            "failure_reasons_top": dict(harness.failure_reasons.most_common(20)),
            "args": vars(args),
        }
        logger.write_summary(summary)
        return summary

    start_ts = time.monotonic()
    round_index = 0
    while True:
        elapsed = time.monotonic() - start_ts
        if elapsed >= args.duration_sec:
            break

        round_index += 1
        round_start = time.monotonic()
        result = await harness.heartbeat_round(f"stability_round_{round_index}")
        round_results.append(result)
        logger.log(
            "INFO",
            "stability round done",
            round=round_index,
            attempts=result["attempts"],
            success=result["success"],
            success_ratio=round(result["success_ratio"], 6),
            p95_ms=round(result["latency_ms"]["p95"], 3),
            alive=result["alive_after_round"],
        )

        round_elapsed = time.monotonic() - round_start
        sleep_sec = max(0.0, args.heartbeat_interval_sec - round_elapsed)
        if sleep_sec > 0:
            await asyncio.sleep(sleep_sec)

        if len(harness.alive_clients) == 0:
            logger.log("WARN", "all connections dropped during stability test", round=round_index)
            break

    merged = merge_rounds(round_results)
    final_alive = len(harness.alive_clients)
    drop_ratio = ((alive_after_connect - final_alive) / alive_after_connect) if alive_after_connect else 1.0
    p95 = float(merged["latency_ms"]["p95"])
    success_ratio = float(merged["success_ratio"])

    pass_ok = (
        connect_ratio >= args.min_connect_ratio
        and success_ratio >= args.min_heartbeat_ratio
        and drop_ratio <= args.max_drop_ratio
        and p95 <= args.latency_threshold_ms
    )

    summary = {
        "mode": "stability",
        "result": "passed" if pass_ok else "failed",
        "alive_after_connect": alive_after_connect,
        "alive_at_end": final_alive,
        "connect_ratio": connect_ratio,
        "drop_ratio": drop_ratio,
        "heartbeat_success_ratio": success_ratio,
        "latency_ms": merged["latency_ms"],
        "failure_reasons_top": dict(harness.failure_reasons.most_common(20)),
        "rounds": round_results,
        "args": vars(args),
    }
    logger.write_summary(summary)
    await harness.close_all()
    return summary


async def run_pingpong_limit(args: argparse.Namespace, logger: RunLogger) -> Dict:
    harness = StressHarness(args, logger)
    check_nofile(logger, args.max_connections)

    stage_target = args.start_connections
    best_stage = 0
    stop_reason = ""
    stage_results: List[Dict] = []

    while stage_target <= args.max_connections:
        logger.log("INFO", "pingpong stage start", stage_target=stage_target, alive=len(harness.alive_clients))
        await harness.add_connections_until(stage_target, args.extra_attempt_ratio)

        alive_count = len(harness.alive_clients)
        connect_ratio = alive_count / stage_target if stage_target else 0.0
        logger.log(
            "INFO",
            "pingpong stage connect result",
            stage_target=stage_target,
            alive=alive_count,
            connect_ratio=round(connect_ratio, 6),
        )

        if connect_ratio < args.min_connect_ratio:
            stop_reason = f"connect_ratio_below_threshold:{connect_ratio:.4f}"
            break

        stage_rounds: List[Dict] = []
        for round_index in range(1, args.rounds_per_stage + 1):
            result = await harness.heartbeat_round(f"pingpong_{stage_target}_round_{round_index}")
            stage_rounds.append(result)
            logger.log(
                "INFO",
                "pingpong round done",
                stage_target=stage_target,
                round=round_index,
                attempts=result["attempts"],
                success=result["success"],
                success_ratio=round(result["success_ratio"], 6),
                p95_ms=round(result["latency_ms"]["p95"], 3),
                alive=result["alive_after_round"],
            )
            await asyncio.sleep(args.ping_interval_ms / 1000.0)

        merged = merge_rounds(stage_rounds)
        fail_ratio = 1.0 - float(merged["success_ratio"])
        p95 = float(merged["latency_ms"]["p95"])
        stage_record = {
            "stage_target": stage_target,
            "alive_after_stage": len(harness.alive_clients),
            "connect_ratio": connect_ratio,
            "fail_ratio": fail_ratio,
            "latency_ms": merged["latency_ms"],
            "rounds": stage_rounds,
        }
        stage_results.append(stage_record)

        if fail_ratio > args.max_fail_ratio:
            stop_reason = f"fail_ratio_exceeds_threshold:{fail_ratio:.4f}"
            break
        if p95 > args.latency_threshold_ms:
            stop_reason = f"p95_exceeds_threshold:{p95:.3f}"
            break

        best_stage = len(harness.alive_clients)
        stage_target += args.step_connections
        await asyncio.sleep(args.stage_interval_sec)

    if not stop_reason:
        stop_reason = "reached_max_connections"

    summary = {
        "mode": "pingpong-limit",
        "stop_reason": stop_reason,
        "best_stable_connections": best_stage,
        "alive_at_end": len(harness.alive_clients),
        "failure_reasons_top": dict(harness.failure_reasons.most_common(20)),
        "stages": stage_results,
        "args": vars(args),
    }
    logger.write_summary(summary)
    await harness.close_all()
    return summary


def run_cmd(logger: RunLogger, command: List[str], env: Optional[Dict[str, str]] = None) -> subprocess.CompletedProcess:
    logger.log("INFO", "execute command", command=" ".join(shlex.quote(part) for part in command))
    return subprocess.run(command, check=False, text=True, capture_output=True, env=env)


def mysql_query(logger: RunLogger, mysql_args: argparse.Namespace, sql: str, use_schema: bool = True) -> subprocess.CompletedProcess:
    command = [
        "mysql",
        "-h",
        mysql_args.mysql_host,
        "-P",
        str(mysql_args.mysql_port),
        "-u",
        mysql_args.mysql_user,
        "-N",
        "-B",
    ]
    if use_schema:
        command.append(mysql_args.schema)
    command.extend(["-e", sql])

    env = os.environ.copy()
    env["MYSQL_PWD"] = mysql_args.mysql_password
    return run_cmd(logger, command, env=env)


def mysql_query_with_retry(logger: RunLogger, mysql_args: argparse.Namespace, sql: str, use_schema: bool = True) -> subprocess.CompletedProcess:
    result = mysql_query(logger, mysql_args, sql, use_schema=use_schema)
    mismatch = "Protocol mismatch" in (result.stderr or "")
    if result.returncode == 0 or not mismatch:
        return result

    if int(mysql_args.mysql_port) == 33060:
        logger.log(
            "WARN",
            "mysql protocol mismatch detected, retry with classic port 3306",
            old_port=mysql_args.mysql_port,
        )
        mysql_args.mysql_port = 3306
        return mysql_query(logger, mysql_args, sql, use_schema=use_schema)

    return result


def mysql_escape(text: str) -> str:
    return text.replace("'", "''")


def sql_literal(value, is_numeric: bool) -> str:
    if value is None:
        return "NULL"
    if is_numeric:
        return str(int(value))
    return f"'{mysql_escape(str(value))}'"


def build_seed_sql(
    routine_type: str,
    routine_name: str,
    param_meta: Sequence[Tuple[str, str]],
    args: argparse.Namespace,
) -> str:
    numeric_types = {
        "int", "integer", "bigint", "smallint", "mediumint", "tinyint",
        "decimal", "numeric", "float", "double", "real",
    }
    values: List[str] = []
    numeric_fallback_idx = 0
    string_fallback_idx = 0

    for param_name, data_type in param_meta:
        name = (param_name or "").lower()
        dtype = (data_type or "").lower()
        is_numeric = dtype in numeric_types
        value = None

        if "start" in name or "begin" in name:
            value = args.user_start
        elif "count" in name or "num" in name or "size" in name or "total" in name:
            value = args.seed_users
        elif "prefix" in name:
            value = args.user_prefix
        elif "pass" in name or "pwd" in name:
            value = args.password
        elif "domain" in name:
            value = args.email_domain

        if value is None:
            if is_numeric:
                if numeric_fallback_idx == 0:
                    value = args.user_start
                elif numeric_fallback_idx == 1:
                    value = args.seed_users
                numeric_fallback_idx += 1
            else:
                if string_fallback_idx == 0:
                    value = args.user_prefix
                elif string_fallback_idx == 1:
                    value = args.password
                elif string_fallback_idx == 2:
                    value = args.email_domain
                string_fallback_idx += 1

        values.append(sql_literal(value, is_numeric=is_numeric))

    call_args = ", ".join(values)
    if routine_type.upper() == "FUNCTION":
        return f"SELECT {routine_name}({call_args});"
    return f"CALL {routine_name}({call_args});"


def run_prepare(args: argparse.Namespace, logger: RunLogger) -> Dict:
    logger.log("INFO", "prepare started", schema=args.schema)
    errors: List[str] = []
    executed_sql = ""

    if not args.skip_switch:
        switch_cmd = ["bash", str(args.switch_script), args.schema]
        switch_result = run_cmd(logger, switch_cmd)
        if switch_result.returncode != 0:
            errors.append("switch_db_failed")
            logger.log(
                "ERROR",
                "switch_db failed",
                returncode=switch_result.returncode,
                stdout=switch_result.stdout,
                stderr=switch_result.stderr,
            )
        else:
            logger.log("INFO", "switch_db success", stdout=switch_result.stdout.strip())

    if args.seed_call:
        executed_sql = args.seed_call
        if not args.dry_run:
            seed_result = mysql_query_with_retry(logger, args, executed_sql, use_schema=True)
            if seed_result.returncode != 0:
                errors.append("seed_sql_failed")
                logger.log(
                    "ERROR",
                    "seed call failed",
                    returncode=seed_result.returncode,
                    stdout=seed_result.stdout,
                    stderr=seed_result.stderr,
                )
            else:
                logger.log("INFO", "seed call success", stdout=seed_result.stdout.strip())
    else:
        routine_name = args.seed_routine
        type_sql = (
            "SELECT ROUTINE_TYPE FROM information_schema.ROUTINES "
            f"WHERE ROUTINE_SCHEMA='{mysql_escape(args.schema)}' "
            f"AND ROUTINE_NAME='{mysql_escape(routine_name)}' LIMIT 1;"
        )
        type_result = mysql_query_with_retry(logger, args, type_sql, use_schema=False)
        if type_result.returncode != 0 or not type_result.stdout.strip():
            errors.append("seed_routine_not_found")
            logger.log(
                "ERROR",
                "seed routine not found",
                returncode=type_result.returncode,
                stdout=type_result.stdout,
                stderr=type_result.stderr,
                routine=routine_name,
                schema=args.schema,
            )
        else:
            routine_type = type_result.stdout.strip().splitlines()[0].strip().upper()
            meta_sql = (
                "SELECT ORDINAL_POSITION, PARAMETER_NAME, DATA_TYPE FROM information_schema.PARAMETERS "
                f"WHERE SPECIFIC_SCHEMA='{mysql_escape(args.schema)}' "
                f"AND SPECIFIC_NAME='{mysql_escape(routine_name)}' "
                "AND ORDINAL_POSITION > 0 "
                "ORDER BY ORDINAL_POSITION;"
            )
            meta_result = mysql_query_with_retry(logger, args, meta_sql, use_schema=False)
            if meta_result.returncode != 0:
                errors.append("seed_param_count_failed")
                logger.log(
                    "ERROR",
                    "failed to inspect routine parameters",
                    returncode=meta_result.returncode,
                    stdout=meta_result.stdout,
                    stderr=meta_result.stderr,
                )
            else:
                param_meta: List[Tuple[str, str]] = []
                for line in (meta_result.stdout or "").splitlines():
                    parts = line.split("\t")
                    if len(parts) < 3:
                        continue
                    parameter_name = parts[1].strip()
                    data_type = parts[2].strip()
                    param_meta.append((parameter_name, data_type))

                executed_sql = build_seed_sql(routine_type, routine_name, param_meta, args)
                logger.log(
                    "INFO",
                    "auto generated seed sql",
                    routine_type=routine_type,
                    param_count=len(param_meta),
                    param_meta=param_meta,
                    sql=executed_sql,
                )
                if not args.dry_run:
                    exec_result = mysql_query_with_retry(logger, args, executed_sql, use_schema=True)
                    if exec_result.returncode != 0:
                        errors.append("seed_exec_failed")
                        logger.log(
                            "ERROR",
                            "seed exec failed",
                            returncode=exec_result.returncode,
                            stdout=exec_result.stdout,
                            stderr=exec_result.stderr,
                        )
                    else:
                        logger.log("INFO", "seed exec success", stdout=exec_result.stdout.strip())

    summary = {
        "mode": "prepare",
        "result": "passed" if not errors else "failed",
        "errors": errors,
        "executed_sql": executed_sql,
        "args": vars(args),
    }
    logger.write_summary(summary)
    return summary


def fill_defaults_from_repo(args: argparse.Namespace):
    repo_root = Path(args.repo_root).resolve()
    gate_config = repo_root / "GateServerWin" / "config.ini"
    status_config = repo_root / "StatusServer" / "config.ini"

    if args.log_root is None:
        args.log_root = str((repo_root / "logs" / "stress").resolve())

    if args.gate_port is None:
        gate_port = ini_get(gate_config, "GateServer", "Port", "8080")
        args.gate_port = int(gate_port or "8080")

    if args.mysql_host is None:
        args.mysql_host = ini_get(status_config, "Mysql", "Host", "127.0.0.1") or "127.0.0.1"
    if args.mysql_port is None:
        mysql_port = ini_get(status_config, "Mysql", "Port", "3306")
        args.mysql_port = int(mysql_port or "3306")
    if args.mysql_user is None:
        args.mysql_user = ini_get(status_config, "Mysql", "User", "root") or "root"
    if args.mysql_password is None:
        args.mysql_password = ini_get(status_config, "Mysql", "Passwd", "") or ""


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="myChat 压测脚本：连接上限、稳定性、10ms pingpong 上限",
    )
    parser.add_argument("--repo-root", default=str(Path(__file__).resolve().parent.parent), help="项目根目录")
    parser.add_argument("--log-root", default=None, help="日志根目录，默认 logs/stress")

    parser.add_argument("--gate-host", default="127.0.0.1", help="GateServer HTTP Host")
    parser.add_argument("--gate-port", type=int, default=None, help="GateServer HTTP Port")
    parser.add_argument("--default-chat-host", default="127.0.0.1", help="当 chathost 为空时的回退值")

    parser.add_argument("--user-prefix", default="stress_u_", help="账号前缀（拼接 index）")
    parser.add_argument("--email-domain", default="stress.local", help="邮箱域名")
    parser.add_argument("--user-start", type=int, default=300000, help="起始账号序号")
    parser.add_argument("--password", default="abc123", help="统一密码")

    parser.add_argument("--connect-timeout", type=float, default=5.0, help="TCP 建连超时（秒）")
    parser.add_argument("--request-timeout", type=float, default=8.0, help="登录/请求超时（秒）")
    parser.add_argument("--heartbeat-timeout", type=float, default=3.0, help="心跳读超时（秒）")
    parser.add_argument("--connect-batch-size", type=int, default=200, help="批量建连并发数")
    parser.add_argument("--connect-batch-interval-ms", type=float, default=0.0, help="每批建连后暂停毫秒，缓解瞬时洪峰")
    parser.add_argument("--heartbeat-batch-size", type=int, default=500, help="心跳并发批次大小")
    parser.add_argument("--gate-login-retries", type=int, default=2, help="Gate 登录重试次数（仅错误码1002/网络抖动）")
    parser.add_argument("--chat-connect-retries", type=int, default=2, help="Chat 建连重试次数（仅超时/reset/oserror_99）")
    parser.add_argument("--retry-backoff-ms", type=float, default=150.0, help="重试退避毫秒")
    parser.add_argument("--extra-attempt-ratio", type=float, default=0.2, help="为目标连接数预留的额外账号尝试比例")
    parser.add_argument("--max-no-progress-batches", type=int, default=3, help="连续无进展批次数后停止继续建连")

    parser.add_argument("--mysql-host", default=None, help="MySQL Host")
    parser.add_argument("--mysql-port", type=int, default=None, help="MySQL Port")
    parser.add_argument("--mysql-user", default=None, help="MySQL User")
    parser.add_argument("--mysql-password", default=None, help="MySQL Password")
    parser.add_argument("--schema", default="stress_mysql", help="压测 Schema")

    subparsers = parser.add_subparsers(dest="command", required=True)

    prepare_parser = subparsers.add_parser("prepare", help="切换到 stress_mysql 并自动批量造号")
    prepare_parser.add_argument("--skip-switch", action="store_true", help="跳过调用 scripts/switch_db.sh")
    prepare_parser.add_argument(
        "--switch-script",
        default=str(Path("scripts") / "switch_db.sh"),
        help="切库脚本路径",
    )
    prepare_parser.add_argument("--seed-users", type=int, default=12000, help="造号总数")
    prepare_parser.add_argument("--seed-routine", default="stress_bulk_seed_users", help="造号存储过程/函数名")
    prepare_parser.add_argument("--seed-call", default="", help="手工指定 SQL（例如 CALL ...）")
    prepare_parser.add_argument("--dry-run", action="store_true", help="仅生成 SQL，不真正执行")

    conn_limit_parser = subparsers.add_parser("conn-limit", help="阶段增压，测试稳定连接上限")
    conn_limit_parser.add_argument("--start-connections", type=int, default=1000)
    conn_limit_parser.add_argument("--step-connections", type=int, default=1000)
    conn_limit_parser.add_argument("--max-connections", type=int, default=30000)
    conn_limit_parser.add_argument("--probe-rounds", type=int, default=2)
    conn_limit_parser.add_argument("--stage-interval-sec", type=float, default=1.0)
    conn_limit_parser.add_argument("--min-connect-ratio", type=float, default=0.98)
    conn_limit_parser.add_argument("--min-heartbeat-ratio", type=float, default=0.99)
    conn_limit_parser.add_argument("--latency-threshold-ms", type=float, default=50.0)

    stability_parser = subparsers.add_parser("stability", help="固定连接数长稳态测试")
    stability_parser.add_argument("--connections", type=int, default=10000)
    stability_parser.add_argument("--duration-sec", type=int, default=300)
    stability_parser.add_argument("--heartbeat-interval-sec", type=float, default=2.0)
    stability_parser.add_argument("--min-connect-ratio", type=float, default=0.98)
    stability_parser.add_argument("--min-heartbeat-ratio", type=float, default=0.99)
    stability_parser.add_argument("--max-drop-ratio", type=float, default=0.01)
    stability_parser.add_argument("--latency-threshold-ms", type=float, default=10.0)

    pingpong_parser = subparsers.add_parser("pingpong-limit", help="10ms pingpong 下连接上限")
    pingpong_parser.add_argument("--start-connections", type=int, default=2000)
    pingpong_parser.add_argument("--step-connections", type=int, default=1000)
    pingpong_parser.add_argument("--max-connections", type=int, default=30000)
    pingpong_parser.add_argument("--rounds-per-stage", type=int, default=5)
    pingpong_parser.add_argument("--ping-interval-ms", type=float, default=10.0)
    pingpong_parser.add_argument("--stage-interval-sec", type=float, default=1.0)
    pingpong_parser.add_argument("--min-connect-ratio", type=float, default=0.98)
    pingpong_parser.add_argument("--max-fail-ratio", type=float, default=0.01)
    pingpong_parser.add_argument("--latency-threshold-ms", type=float, default=10.0)

    return parser


def command_mode_name(command: str) -> str:
    return command.replace("-", "_")


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    fill_defaults_from_repo(args)

    log_root = Path(args.log_root).resolve()
    logger = RunLogger(mode=command_mode_name(args.command), log_root=log_root)
    logger.log("INFO", "run start", command=args.command, log_dir=str(logger.run_dir))

    if args.command == "prepare":
        if not Path(args.switch_script).is_absolute():
            args.switch_script = str((Path(args.repo_root) / args.switch_script).resolve())
        summary = run_prepare(args, logger)
        logger.log("INFO", "run finished", result=summary.get("result", "unknown"))
        return 0 if summary.get("result") == "passed" else 1

    try:
        if args.command == "conn-limit":
            summary = asyncio.run(run_conn_limit(args, logger))
        elif args.command == "stability":
            summary = asyncio.run(run_stability(args, logger))
        elif args.command == "pingpong-limit":
            summary = asyncio.run(run_pingpong_limit(args, logger))
        else:
            parser.error(f"unsupported command: {args.command}")
            return 2
    except KeyboardInterrupt:
        logger.log("WARN", "interrupted by user")
        return 130

    logger.log(
        "INFO",
        "run finished",
        mode=args.command,
        summary_file=str(logger.summary_file),
        stop_reason=summary.get("stop_reason", ""),
        result=summary.get("result", ""),
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
