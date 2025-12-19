# Script para monitorar Arduino na COM12 e iniciar o webserver automaticamente
# Detecta quando a porta COM12 consegue abrir (Arduino conectado)

$caminhoApp = "C:\Users\Luisao\Downloads\supervisório-de-solda-profissional"
$portaAlvo = 'COM12'
$detectado = $false
$tentativasConsecutivas = 0

Write-Host "========================================" 
Write-Host "MONITOR DE CONEXAO ARDUINO"
Write-Host "========================================"
Write-Host "Porta: $portaAlvo"
Write-Host "Aguardando conexao do Arduino..."
Write-Host "Pressione Ctrl+C para parar"
Write-Host ""

while ($true) {
    $portaDisponivel = $false
    
    try {
        # Tenta abrir a porta POR UM TEMPO MUITO CURTO
        $serial = New-Object System.IO.Ports.SerialPort($portaAlvo, 115200, 'None', 8, 'One')
        $serial.ReadTimeout = 100
        $serial.Open()
        $portaDisponivel = $true
        $tentativasConsecutivas++
        
        # FECHA E LIBERA IMEDIATAMENTE para nao bloquear o navegador
        $serial.Close()
        $serial.Dispose()
        
        # Se conseguiu abrir 3 vezes consecutivas, consideramos conectado
        if ($tentativasConsecutivas -ge 3 -and -not $detectado) {
            Write-Host ""
            Write-Host "[$(Get-Date -Format 'HH:mm:ss')] Arduino DETECTADO na porta $portaAlvo"
            Write-Host ""
            Write-Host "Iniciando o webserver..."
            
            # Inicia npm run dev em nova janela
            Start-Process -FilePath "powershell" -ArgumentList "-Command", "cd '$caminhoApp'; npm run dev" -WindowStyle Normal
            
            Write-Host ""
            Write-Host "Webserver iniciado!"
            Write-Host "Acesse: http://localhost:3000/"
            Write-Host ""
            Write-Host "Monitor continuara rodando..."
            
            $detectado = $true
        }
        
    } catch {
        # Porta nao consegue abrir
        $tentativasConsecutivas = 0
    }
    
    Start-Sleep -Milliseconds 500
}
