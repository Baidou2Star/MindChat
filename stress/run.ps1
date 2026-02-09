param(
    [string]$Config = "stress/scenarios.json",
    [string]$Scenario = "",
    [int]$Seed = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path $Config)) {
    throw "Config not found: $Config"
}

$cfg = Get-Content $Config -Raw | ConvertFrom-Json
if (-not $Scenario) {
    $Scenario = $cfg.defaultScenario
}
if (-not $Scenario) {
    throw "No scenario specified and defaultScenario is missing."
}

$scn = $cfg.scenarios | Where-Object { $_.name -eq $Scenario }
if (-not $scn) {
    $names = ($cfg.scenarios | ForEach-Object { $_.name }) -join ", "
    throw "Scenario not found: $Scenario. Available: $names"
}

$baseUrl = $cfg.baseUrl
if (-not $baseUrl) {
    throw "baseUrl is missing in config."
}

$timeoutSeconds = 10
if ($cfg.requestTimeoutSeconds) {
    $timeoutSeconds = [int]$cfg.requestTimeoutSeconds
}

$stages = $cfg.stages
if (-not $stages) {
    throw "stages is missing in config."
}

$random = if ($Seed -ne 0) { [System.Random]::new($Seed) } else { [System.Random]::new() }

function Get-WeightedRequest([array]$requests, [System.Random]$rng) {
    $total = 0
    foreach ($r in $requests) {
        $w = if ($r.weight) { [int]$r.weight } else { 1 }
        $total += $w
    }
    if ($total -le 0) { return $requests[0] }
    $roll = $rng.Next(1, $total + 1)
    $acc = 0
    foreach ($r in $requests) {
        $w = if ($r.weight) { [int]$r.weight } else { 1 }
        $acc += $w
        if ($roll -le $acc) { return $r }
    }
    return $requests[0]
}

function Pick-Body([object]$req, [System.Random]$rng) {
    if ($req.PSObject.Properties.Name -contains "bodyPool") {
        $pool = $req.bodyPool
        if (-not $pool -or $pool.Count -eq 0) { return $null }
        $idx = $rng.Next(0, $pool.Count)
        return $pool[$idx]
    }
    if ($req.PSObject.Properties.Name -contains "body") {
        return $req.body
    }
    return $null
}

function Get-Percentile([double[]]$arr, [int]$p) {
    if ($arr.Length -eq 0) { return 0 }
    $idx = [math]::Ceiling(($p / 100.0) * $arr.Length) - 1
    if ($idx -lt 0) { $idx = 0 }
    if ($idx -ge $arr.Length) { $idx = $arr.Length - 1 }
    return $arr[$idx]
}

function Merge-Headers([hashtable]$base, [object]$extra) {
    $h = @{}
    foreach ($k in $base.Keys) { $h[$k] = $base[$k] }
    if ($extra) {
        foreach ($prop in $extra.PSObject.Properties) {
            $h[$prop.Name] = [string]$prop.Value
        }
    }
    return $h
}

$defaultHeaders = @{}
if ($cfg.defaultHeaders) {
    foreach ($p in $cfg.defaultHeaders.PSObject.Properties) {
        $defaultHeaders[$p.Name] = [string]$p.Value
    }
}

$client = [System.Net.Http.HttpClient]::new()
$client.Timeout = [TimeSpan]::FromSeconds($timeoutSeconds)

$totalLat = New-Object System.Collections.Concurrent.ConcurrentBag[double]
$totalStatuses = New-Object System.Collections.Concurrent.ConcurrentDictionary[int,int]
$totalSuccess = 0
$totalFail = 0

Write-Host "Scenario: $Scenario"
Write-Host "BaseUrl:  $baseUrl"

foreach ($stage in $stages) {
    $stageName = if ($stage.name) { $stage.name } else { "stage" }
    $duration = [int]$stage.durationSeconds
    $concurrency = [int]$stage.targetConcurrency

    if ($duration -le 0 -or $concurrency -le 0) {
        Write-Host "Skip stage '$stageName' (duration/concurrency invalid)"
        continue
    }

    $latencies = New-Object System.Collections.Concurrent.ConcurrentBag[double]
    $statuses = New-Object System.Collections.Concurrent.ConcurrentDictionary[int,int]
    $success = 0
    $fail = 0

    $cts = New-Object System.Threading.CancellationTokenSource
    $cts.CancelAfter([TimeSpan]::FromSeconds($duration))
    $token = $cts.Token

    $tasks = @()
    for ($i = 0; $i -lt $concurrency; $i++) {
        $tasks += [System.Threading.Tasks.Task]::Run({
            while (-not $token.IsCancellationRequested) {
                $req = Get-WeightedRequest $scn.requests $random
                $method = $req.method
                $path = $req.path
                $url = "$baseUrl$path"

                $msg = [System.Net.Http.HttpRequestMessage]::new([System.Net.Http.HttpMethod]::new($method), $url)

                $headers = Merge-Headers $defaultHeaders $req.headers
                foreach ($k in $headers.Keys) {
                    $null = $msg.Headers.TryAddWithoutValidation($k, $headers[$k])
                }

                $bodyObj = Pick-Body $req $random
                if ($bodyObj -ne $null -and $method -ne "GET") {
                    $bodyJson = $bodyObj | ConvertTo-Json -Compress
                    $msg.Content = [System.Net.Http.StringContent]::new($bodyJson, [System.Text.Encoding]::UTF8, "application/json")
                }

                $sw = [System.Diagnostics.Stopwatch]::StartNew()
                try {
                    $resp = $client.SendAsync($msg, $token).GetAwaiter().GetResult()
                    $sw.Stop()
                    $latencies.Add($sw.Elapsed.TotalMilliseconds)
                    $totalLat.Add($sw.Elapsed.TotalMilliseconds)

                    $code = [int]$resp.StatusCode
                    $statuses.AddOrUpdate($code, 1, { param($k, $v) $v + 1 }) | Out-Null
                    $totalStatuses.AddOrUpdate($code, 1, { param($k, $v) $v + 1 }) | Out-Null

                    if ($code -ge 200 -and $code -lt 300) {
                        [System.Threading.Interlocked]::Increment([ref]$success) | Out-Null
                        [System.Threading.Interlocked]::Increment([ref]$totalSuccess) | Out-Null
                    } else {
                        [System.Threading.Interlocked]::Increment([ref]$fail) | Out-Null
                        [System.Threading.Interlocked]::Increment([ref]$totalFail) | Out-Null
                    }
                } catch {
                    $sw.Stop()
                    $latencies.Add($sw.Elapsed.TotalMilliseconds)
                    $totalLat.Add($sw.Elapsed.TotalMilliseconds)
                    [System.Threading.Interlocked]::Increment([ref]$fail) | Out-Null
                    [System.Threading.Interlocked]::Increment([ref]$totalFail) | Out-Null
                }

                if ($req.PSObject.Properties.Name -contains "thinkTimeMs") {
                    Start-Sleep -Milliseconds ([int]$req.thinkTimeMs)
                }
            }
        }, $token)
    }

    [System.Threading.Tasks.Task]::WaitAll($tasks)

    $latArr = $latencies.ToArray() | Sort-Object
    $count = $latArr.Length
    $avg = if ($count -gt 0) { ($latArr | Measure-Object -Average).Average } else { 0 }
    $min = if ($count -gt 0) { $latArr[0] } else { 0 }
    $max = if ($count -gt 0) { $latArr[$count - 1] } else { 0 }
    $p50 = Get-Percentile $latArr 50
    $p90 = Get-Percentile $latArr 90
    $p99 = Get-Percentile $latArr 99

    $rps = [math]::Round(($count / $duration), 2)
    $ok = $success
    $bad = $fail

    Write-Host ""
    Write-Host "Stage: $stageName"
    Write-Host "Duration: $duration s, Concurrency: $concurrency"
    Write-Host "Requests: $count, RPS: $rps, OK: $ok, FAIL: $bad"
    Write-Host ("Latency(ms) min/avg/p50/p90/p99/max: {0:N2}/{1:N2}/{2:N2}/{3:N2}/{4:N2}/{5:N2}" -f $min, $avg, $p50, $p90, $p99, $max)
    if ($statuses.Count -gt 0) {
        $codes = $statuses.Keys | Sort-Object
        $summary = $codes | ForEach-Object { "$_=$($statuses[$_])" }
        Write-Host "Status: $([string]::Join(", ", $summary))"
    }
}

$allLat = $totalLat.ToArray() | Sort-Object
$allCount = $allLat.Length
$allAvg = if ($allCount -gt 0) { ($allLat | Measure-Object -Average).Average } else { 0 }
$allMin = if ($allCount -gt 0) { $allLat[0] } else { 0 }
$allMax = if ($allCount -gt 0) { $allLat[$allCount - 1] } else { 0 }
$allP50 = Get-Percentile $allLat 50
$allP90 = Get-Percentile $allLat 90
$allP99 = Get-Percentile $allLat 99

Write-Host ""
Write-Host "Overall"
Write-Host "Requests: $allCount, OK: $totalSuccess, FAIL: $totalFail"
Write-Host ("Latency(ms) min/avg/p50/p90/p99/max: {0:N2}/{1:N2}/{2:N2}/{3:N2}/{4:N2}/{5:N2}" -f $allMin, $allAvg, $allP50, $allP90, $allP99, $allMax)
if ($totalStatuses.Count -gt 0) {
    $codes = $totalStatuses.Keys | Sort-Object
    $summary = $codes | ForEach-Object { "$_=$($totalStatuses[$_])" }
    Write-Host "Status: $([string]::Join(", ", $summary))"
}
