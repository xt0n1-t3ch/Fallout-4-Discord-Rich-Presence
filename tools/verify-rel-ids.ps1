[CmdletBinding()]
param(
    [string]$PluginsDir,
    [uint64[]]$Ids,
    [string[]]$Runtimes
)

$ErrorActionPreference = 'Stop'

$script:DefaultPluginsDir = Join-Path ${env:ProgramFiles(x86)} 'Steam\steamapps\common\Fallout 4\Data\F4SE\Plugins'

$script:ReferencePluginTargets = [ordered]@{
    BGSAnimSoundStateManager_Update_PreNG  = [uint64]1426208
    TESPlacementSource_GetEventSource_PreNG = [uint64]1067439
}

$script:KnownRuntimes = @(
    '1-10-163-0',
    '1-10-984-0',
    '1-11-169-0',
    '1-11-191-0'
)

if (-not $PluginsDir) { $PluginsDir = $script:DefaultPluginsDir }
if (-not $Ids)        { $Ids        = [uint64[]]$script:ReferencePluginTargets.Values }
if (-not $Runtimes)   { $Runtimes   = $script:KnownRuntimes }

function Read-IdMap {
    param([string]$Path)
    $bytes  = [System.IO.File]::ReadAllBytes($Path)
    $count  = [System.BitConverter]::ToUInt64($bytes, 0)
    $cursor = 8
    $map    = New-Object 'System.Collections.Generic.Dictionary[uint64,uint64]'
    for ([int]$i = 0; $i -lt $count; $i++) {
        $id     = [System.BitConverter]::ToUInt64($bytes, $cursor); $cursor += 8
        $offset = [System.BitConverter]::ToUInt64($bytes, $cursor); $cursor += 8
        $map[$id] = $offset
    }
    [pscustomobject]@{ Count = $count; Map = $map }
}

foreach ($runtime in $Runtimes) {
    $bin = Join-Path $PluginsDir ("version-{0}.bin" -f $runtime)
    if (-not (Test-Path -LiteralPath $bin)) {
        Write-Output ("MISSING bin: {0}" -f $bin)
        continue
    }
    $parsed = Read-IdMap -Path $bin
    Write-Output ("[{0}] count={1}" -f $runtime, $parsed.Count)
    foreach ($id in $Ids) {
        if ($parsed.Map.ContainsKey([uint64]$id)) {
            $rva = $parsed.Map[[uint64]$id]
            Write-Output ("  OK   REL::ID({0}) -> RVA 0x{1:x}" -f $id, $rva)
        } else {
            Write-Output ("  MISS REL::ID({0})" -f $id)
        }
    }
}
