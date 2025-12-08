# Projeto Soldagem Arduino

Sistema de controle e monitoramento para soldagem utilizando Arduino e supervisório Python.

## 📁 Estrutura do Projeto

Para organizar o projeto, crie a seguinte estrutura de pastas e arquivos:

```
projeto-soldagem-arduino/
│
├── README.md                          # Este arquivo
│
├── docs/                              # Documentação do projeto
│   ├── pinagem.md                     # Tabela de conexões e pinos
│   ├── protocolo_serial.md            # Documentação da comunicação serial
│   └── guia_usuario.pdf               # Manual do usuário (entrega final)
│
├── hardware/                          # Arquivos do circuito eletrônico
│   ├── circuito.pdsprj                # Projeto Proteus
│   └── esquematico.png                # Imagem do esquemático
│
├── firmware/                          # Código do Arduino
│   ├── main.ino                       # Código principal integrado
│   ├── aquisicao.h                    # Cabeçalho - Aquisição de dados
│   ├── aquisicao.cpp                  # Implementação - Leitura de sensores
│   ├── interface.h                    # Cabeçalho - Interface (LCD/Display)
│   ├── interface.cpp                  # Implementação - Controle de display
│   ├── comunicacao.h                  # Cabeçalho - Comunicação serial
│   ├── comunicacao.cpp                # Implementação - Protocolo serial
│   ├── controle.h                     # Cabeçalho - Controle de atuadores
│   └── controle.cpp                   # Implementação - Relés e PWM
│
├── supervisorio/                      # Sistema supervisório Python
│   ├── main.py                        # Aplicação principal (Tkinter)
│   ├── serial_comm.py                 # Módulo de comunicação serial
│   ├── dashboard.py                   # Interface gráfica
│   ├── relatorio.py                   # Gerador de relatórios PDF
│   └── requirements.txt               # Dependências Python
│
└── testes/                            # Testes unitários e validação
    ├── teste_lm35.ino                 # Teste do sensor de temperatura
    ├── teste_display.ino              # Teste do display LCD
    └── teste_serial.py                # Teste da comunicação serial
```

## 🔧 Ferramentas Necessárias

- **Arduino IDE**: Para desenvolvimento do firmware
- **Proteus**: Para simulação do circuito
- **Python 3.x**: Para o sistema supervisório
- **VS Code** (opcional): Editor de código
- **Git**: Para controle de versão
