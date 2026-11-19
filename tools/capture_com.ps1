param(
	[string]$PortName = "COM3",
	[ValidateRange(1, 3600)]
	[int]$DurationSeconds = 5,
	[string]$OutputPath = ".\capture.csv"
)

$serial = New-Object System.IO.Ports.SerialPort
$serial.PortName = $PortName
$serial.BaudRate = 115200
$serial.DataBits = 8
$serial.Parity = [System.IO.Ports.Parity]::None
$serial.StopBits = [System.IO.Ports.StopBits]::One
$serial.ReadTimeout = 100
$serial.WriteTimeout = 1000

$writer = $null
$recording = $false
$rows = 0

try {
	$serial.Open()
	$serial.DiscardInBuffer()
	$writer = [System.IO.File]::CreateText($OutputPath)
	$writer.WriteLine("mean_mV,delta_mV")

	$serial.Write("CAPTURE`n")
	$recording = $true
	$watch = [System.Diagnostics.Stopwatch]::StartNew()
	$readBuffer = New-Object byte[] 256
	$payload = New-Object byte[] 4
	$syncState = 0
	$payloadIndex = 0

	while ($watch.Elapsed.TotalSeconds -lt $DurationSeconds) {
		try {
			$readCount = $serial.Read($readBuffer, 0, $readBuffer.Length)
		}
		catch [System.TimeoutException] {
			continue
		}

		for ($i = 0; $i -lt $readCount; $i++) {
			$value = $readBuffer[$i]
			if ($syncState -eq 0) {
				if ($value -eq 0xA5) { $syncState = 1 }
			}
			elseif ($syncState -eq 1) {
				if ($value -eq 0x5A) {
					$syncState = 2
					$payloadIndex = 0
				}
				elseif ($value -ne 0xA5) {
					$syncState = 0
				}
			}
			else {
				$payload[$payloadIndex] = $value
				$payloadIndex++
				if ($payloadIndex -eq 4) {
					$meanMv = [System.BitConverter]::ToUInt16($payload, 0)
					$deltaMv = [System.BitConverter]::ToInt16($payload, 2)
					$writer.WriteLine("{0},{1}" -f $meanMv, $deltaMv)
					$rows++
					$syncState = 0
				}
			}
		}
	}

	$serial.Write("R`n")
	$recording = $false
	$writer.Flush()
	Write-Host "$rows Messwerte gespeichert: $([System.IO.Path]::GetFullPath($OutputPath))"
}
finally {
	try {
		if ($recording -and $serial.IsOpen) {
			$serial.Write("R`n")
		}
	}
	finally {
		if ($null -ne $writer) {
			$writer.Dispose()
		}
		if ($serial.IsOpen) {
			$serial.Close()
		}
	}
}
