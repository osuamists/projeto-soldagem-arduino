
import React, { useState, useEffect, useRef } from 'react';
import { 
  Activity, Zap, Thermometer, Wind, Settings, AlertOctagon, 
  Cpu, Power, Terminal, ShieldCheck, Database, Sliders,
  TrendingUp, Info, AlertTriangle, Play, Square,
  HardDrive, Network, User, Link, Link2Off, ChevronRight,
  Usb
} from 'lucide-react';
import { 
  XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, 
  AreaChart, Area
} from 'recharts';
import { GoogleGenAI } from "@google/genai";
import { WeldingData, SystemState } from './types';

// Componente de Medição Industrial - Visualização de Alta Precisão
const TelemetryNode: React.FC<{ label: string; value: string | number; unit: string; status: 'normal' | 'warning' | 'error'; icon: any }> = ({ label, value, unit, status, icon: Icon }) => (
  <div className="scada-panel p-4 rounded-lg flex flex-col justify-between border-l-4 transition-all duration-300" 
       style={{ borderLeftColor: status === 'normal' ? '#10b981' : status === 'warning' ? '#f59e0b' : '#ef4444' }}>
    <div className="flex justify-between items-start">
      <span className="text-[10px] font-bold text-slate-500 uppercase tracking-widest">{label}</span>
      <Icon className={`w-4 h-4 ${status === 'normal' ? 'text-slate-600' : 'text-current animate-pulse'}`} />
    </div>
    <div className="mt-2 flex items-baseline gap-2">
      <span className="text-4xl font-mono font-bold text-slate-100">{value}</span>
      <span className="text-xs font-medium text-slate-500">{unit}</span>
    </div>
    <div className="mt-2 h-1.5 w-full bg-slate-800 rounded-full overflow-hidden">
      <div className={`h-full transition-all duration-700 ease-out ${status === 'normal' ? 'bg-emerald-500 shadow-[0_0_8px_#10b981]' : status === 'warning' ? 'bg-amber-500 shadow-[0_0_8px_#f59e0b]' : 'bg-red-500 shadow-[0_0_8px_#ef4444]'}`} 
           style={{ width: `${Math.min(100, (Number(value) / (label.includes('Voltage') ? 90 : 450)) * 100)}%` }}></div>
    </div>
  </div>
);

const App: React.FC = () => {
  const [dataHistory, setDataHistory] = useState<WeldingData[]>([]);
  const [currentData, setCurrentData] = useState<WeldingData>({
    tensao: 0, corrente: 0, temperatura: 0, fluxo: 0, rpm: 0, tensaoPico: 0, correntePico: 0, timestamp: Date.now()
  });
  const [state, setState] = useState<SystemState>({
    relesSistemaLigado: false, relesModoSMAW: true, alarmeGlobalAtivo: false, emergenciaAtiva: false
  });
  const [eventLog, setEventLog] = useState<{type: 'INFO' | 'WARN' | 'CRIT', msg: string, time: string}[]>([]);
  const [aiInsight, setAiInsight] = useState<string>("");
  const [isAiLoading, setIsAiLoading] = useState(false);
  
  // Gerenciamento da Porta Serial
  const [serialConnected, setSerialConnected] = useState(false);
  const readerRef = useRef<ReadableStreamDefaultReader | null>(null);
  const portRef = useRef<any>(null);

  const triggerLog = (type: 'INFO' | 'WARN' | 'CRIT', msg: string) => {
    setEventLog(prev => [{ type, msg, time: new Date().toLocaleTimeString() }, ...prev].slice(0, 30));
    if (type === 'CRIT' && !state.alarmeGlobalAtivo) {
      setState(s => ({ ...s, alarmeGlobalAtivo: true, relesSistemaLigado: false }));
    }
  };

  // Tenta reconectar a portas já autorizadas automaticamente ao carregar
  useEffect(() => {
    const autoCheck = async () => {
      if ("serial" in navigator) {
        // @ts-ignore
        const ports = await navigator.serial.getPorts();
        if (ports.length > 0) {
          triggerLog('INFO', 'Hardware detectado. Clique em "CONECTAR USB" para iniciar fluxo.');
        }
      }
    };
    autoCheck();
  }, []);

  // --- Web Serial API - O Coração da Conexão ---
  const handleConnect = async () => {
    try {
      if (!("serial" in navigator)) {
        alert("Seu navegador não suporta conexão direta com USB. Use o Chrome ou Edge.");
        return;
      }

      // @ts-ignore
      const port = await navigator.serial.requestPort();
      await port.open({ baudRate: 115200 }); // Sincronizado com Serial.begin(115200) do Arduino
      portRef.current = port;
      setSerialConnected(true);
      triggerLog('INFO', 'CONEXÃO ESTABELECIDA: Arduino Mega 2560 online.');

      const textDecoder = new TextDecoderStream();
      port.readable.pipeTo(textDecoder.writable);
      const reader = textDecoder.readable.getReader();
      readerRef.current = reader;

      let buffer = "";
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        
        buffer += value;
        const lines = buffer.split("\n");
        buffer = lines.pop() || "";

        for (const line of lines) {
          if (line.trim()) processSerialLine(line.trim());
        }
      }
    } catch (err) {
      console.error(err);
      setSerialConnected(false);
      triggerLog('WARN', 'Conexão USB interrompida ou negada.');
    }
  };

  const processSerialLine = (line: string) => {
    // Parser para JSON do Arduino:
    // {"tensao":47.2,"corrente":123.4,"temperatura":45.6,"fluxo":8.2,"rpm":1500,"modo":1,"sistema":1,"alarme":0}
    try {
      const trimmed = line.trim();
      console.log("Serial recebido:", trimmed); // DEBUG
      
      const jsonData = JSON.parse(trimmed);
      console.log("JSON parseado:", jsonData); // DEBUG
      
      // Validar se tem os campos essenciais
      if (typeof jsonData.tensao === 'undefined') {
        console.warn("Faltam campos no JSON"); // DEBUG
        return;
      }
      
      const v = parseFloat(jsonData.tensao) || 0;
      const a = parseFloat(jsonData.corrente) || 0;
      const t = parseFloat(jsonData.temperatura) || 0;
      const f = parseFloat(jsonData.fluxo) || 0;
      const r = parseFloat(jsonData.rpm) || 0;
      const modo = parseInt(jsonData.modo) || 0;
      const sist = parseInt(jsonData.sistema) || 0;
      const alarm = parseInt(jsonData.alarme) || 0;

      console.log(`Valores: V=${v}, A=${a}, T=${t}`); // DEBUG

      setCurrentData(prev => {
        const update = {
          tensao: v,
          corrente: a,
          temperatura: t,
          fluxo: f,
          rpm: r,
          tensaoPico: Math.max(prev.tensaoPico, v),
          correntePico: Math.max(prev.correntePico, a),
          timestamp: Date.now()
        };

        setDataHistory(h => [...h.slice(-50), update]);

        // Atualizar estado do sistema
        setState(s => ({
          ...s,
          relesModoSMAW: modo === 1,
          relesSistemaLigado: sist === 1,
          alarmeGlobalAtivo: alarm === 1
        }));

        // Segurança em tempo real
        if (v > 90) triggerLog('CRIT', `ALERTA: Tensão ${v}V acima do limite de 90V!`);
        if (a > 450) triggerLog('CRIT', `ALERTA: Corrente ${a}A acima do limite de 450A!`);
        if (t > 80) triggerLog('CRIT', `ALERTA: Temperatura ${t}°C crítica!`);

        return update;
      });
    } catch (e) {
      console.error("Erro parseando JSON:", e, "Linha:", line); // DEBUG
      // Ignora linhas que não são JSON válido (ruído na serial)
    }
  };

  const valClean = (v: string) => v.replace(/[^0-9.]/g, '');

  const handleDisconnect = async () => {
    if (readerRef.current) {
      await readerRef.current.cancel();
      setSerialConnected(false);
      triggerLog('INFO', 'USB desconectado pelo usuário.');
    }
  };

  const handleAiDiagnostic = async () => {
    setIsAiLoading(true);
    try {
      const ai = new GoogleGenAI({ apiKey: process.env.API_KEY });
      const prompt = `ENGENHEIRO DE SOLDAGEM SCADA:
      Status Hardware: ${serialConnected ? 'REAL_TIME_USB' : 'SIMULATION'}
      Telemetria: Volts=${currentData.tensao}, Amps=${currentData.corrente}, Temp=${currentData.temperatura}C, Gás=${currentData.fluxo}L/min.
      Modo Processo: ${state.relesModoSMAW ? 'SMAW (Eletrodo)' : 'MIG/MAG'}.
      Analise a estabilidade do arco elétrico e dê 3 recomendações técnicas rápidas.`;

      const res = await ai.models.generateContent({ model: 'gemini-3-flash-preview', contents: prompt });
      setAiInsight(res.text || "");
    } catch (e) {
      setAiInsight("Falha na análise neural.");
    } finally {
      setIsAiLoading(false);
    }
  };

  return (
    <div className="flex flex-col h-screen bg-[#0a0c10] overflow-hidden text-slate-300 grid-bg select-none">
      
      {/* SCADA TOPBAR */}
      <nav className="h-16 bg-[#11141b] border-b border-slate-800 px-8 flex items-center justify-between z-50 shadow-2xl">
        <div className="flex items-center gap-10">
          <div className="flex items-center gap-3">
            <div className="bg-blue-600 p-2 rounded-lg shadow-[0_0_15px_rgba(37,99,235,0.4)]">
              <Usb className="w-5 h-5 text-white" />
            </div>
            <div>
              <h1 className="font-black tracking-tighter text-white text-lg leading-none uppercase">WeldMaster <span className="text-blue-500 font-light italic">Core</span></h1>
              <p className="text-[9px] text-slate-500 font-bold tracking-[0.2em] uppercase mt-1">Industrial Interface v3.0</p>
            </div>
          </div>
          
          <div className="h-8 w-px bg-slate-800/50"></div>
          
          <div className="flex items-center gap-8">
            <div className="flex flex-col">
              <span className="text-[9px] font-bold text-slate-600 uppercase mb-1">USB Status</span>
              <div className="flex items-center gap-2">
                <span className={`h-2 w-2 rounded-full ${serialConnected ? 'bg-emerald-500 shadow-[0_0_10px_#10b981] animate-pulse' : 'bg-slate-700'}`}></span>
                <span className={`text-[10px] font-mono font-bold ${serialConnected ? 'text-emerald-500' : 'text-slate-500'}`}>
                  {serialConnected ? 'HARDWARE_LINKED' : 'OFFLINE'}
                </span>
              </div>
            </div>
            <div className="flex flex-col">
              <span className="text-[9px] font-bold text-slate-600 uppercase mb-1">Power System</span>
              <div className="flex items-center gap-2">
                <span className={`h-2 w-2 rounded-full ${state.relesSistemaLigado ? 'bg-blue-500 shadow-[0_0_10px_#3b82f6]' : 'bg-slate-700'}`}></span>
                <span className={`text-[10px] font-mono font-bold ${state.relesSistemaLigado ? 'text-blue-400' : 'text-slate-500'}`}>
                  {state.relesSistemaLigado ? 'ACTIVE_LOAD' : 'IDLE'}
                </span>
              </div>
            </div>
          </div>
        </div>

        <div className="flex items-center gap-6">
          <div className="flex bg-black/40 p-1 rounded-xl border border-slate-800/50 shadow-inner">
            <button 
              onClick={() => setState(s => ({...s, relesModoSMAW: true}))}
              className={`px-6 py-2 text-[10px] font-black rounded-lg transition-all ${state.relesModoSMAW ? 'bg-blue-600 text-white shadow-lg' : 'text-slate-500 hover:text-slate-300'}`}
            >SMAW</button>
            <button 
              onClick={() => setState(s => ({...s, relesModoSMAW: false}))}
              className={`px-6 py-2 text-[10px] font-black rounded-lg transition-all ${!state.relesModoSMAW ? 'bg-blue-600 text-white shadow-lg' : 'text-slate-500 hover:text-slate-300'}`}
            >MIG/MAG</button>
          </div>
          
          <button 
            onClick={() => {
              if (state.alarmeGlobalAtivo) return;
              setState(s => ({...s, relesSistemaLigado: !s.relesSistemaLigado}));
            }}
            disabled={state.alarmeGlobalAtivo}
            className={`flex items-center gap-3 px-8 py-2.5 rounded-xl text-xs font-black uppercase tracking-widest transition-all ${
              state.relesSistemaLigado ? 'bg-red-600 text-white shadow-[0_0_20px_rgba(220,38,38,0.4)] hover:bg-red-500' : 'bg-emerald-600 text-white hover:bg-emerald-500 shadow-[0_0_20px_rgba(5,150,105,0.3)]'
            }`}
          >
            {state.relesSistemaLigado ? <Square className="w-4 h-4 fill-current" /> : <Play className="w-4 h-4 fill-current" />}
            {state.relesSistemaLigado ? 'Halt' : 'Start'}
          </button>
        </div>
      </nav>

      <div className="flex-1 flex overflow-hidden">
        
        {/* SIDEBAR DE CONTROLE RÁPIDO */}
        <aside className="w-80 bg-[#11141b] border-r border-slate-800 p-6 flex flex-col gap-8 shadow-xl z-40">
          <section>
            <h3 className="text-[10px] font-black text-slate-500 uppercase tracking-[0.2em] mb-4">USB Interface</h3>
            <button 
              onClick={serialConnected ? handleDisconnect : handleConnect}
              className={`w-full flex items-center justify-center gap-3 py-4 rounded-xl border-2 font-black text-xs uppercase tracking-widest transition-all ${
                serialConnected 
                ? 'bg-red-500/5 border-red-500/20 text-red-500 hover:bg-red-500/10' 
                : 'bg-blue-600 border-blue-500 text-white shadow-[0_10px_20px_rgba(37,99,235,0.3)] hover:scale-[1.02]'
              }`}
            >
              {serialConnected ? <Link2Off className="w-5 h-5" /> : <Link className="w-5 h-5" />}
              {serialConnected ? 'Desconectar USB' : 'Conectar USB'}
            </button>
            <p className="mt-4 text-[9px] text-slate-600 leading-relaxed font-bold uppercase italic">
              Conecte o Arduino Mega 2560 na porta USB e clique acima para iniciar a leitura.
            </p>
          </section>

          <section className="bg-black/20 rounded-xl border border-slate-800 p-5 space-y-4">
             <div className="flex justify-between items-center text-[10px] font-mono">
                <span className="text-slate-600 font-bold">PORTA:</span>
                <span className="text-blue-400 font-bold">{serialConnected ? "SERIAL_BUS_0" : "NOT_FOUND"}</span>
             </div>
             <div className="flex justify-between items-center text-[10px] font-mono">
                <span className="text-slate-600 font-bold">BITRATE:</span>
                <span className="text-slate-400 font-bold">115200 BPS</span>
             </div>
             <div className="flex justify-between items-center text-[10px] font-mono">
                <span className="text-slate-600 font-bold">PARIDADE:</span>
                <span className="text-slate-400 font-bold">NONE</span>
             </div>
          </section>

          <section className="mt-auto">
            <div className="p-5 bg-blue-600/5 border border-blue-600/10 rounded-xl">
               <div className="flex items-center gap-2 mb-3 text-blue-500">
                  <ShieldCheck className="w-4 h-4" />
                  <span className="text-[10px] font-black uppercase tracking-widest">Protocolo Seguro</span>
               </div>
               <p className="text-[10px] text-slate-500 leading-normal font-medium">
                  Monitoramento contínuo dos pinos 30 e 32 via Hardware Int. Baudrate fixado em 115k para baixa latência.
               </p>
            </div>
          </section>
        </aside>

        {/* DASHBOARD PRINCIPAL */}
        <main className="flex-1 overflow-y-auto p-10 flex flex-col gap-10">
          
          {state.alarmeGlobalAtivo && (
            <div className="bg-red-900/30 border-2 border-red-600 border-dashed p-8 rounded-2xl flex items-center justify-between animate-pulse shadow-[0_0_40px_rgba(220,38,38,0.2)]">
              <div className="flex items-center gap-6">
                <AlertOctagon className="w-12 h-12 text-red-500" />
                <div>
                  <h2 className="text-red-500 font-black text-xl uppercase tracking-tighter">Interrupção Crítica de Hardware</h2>
                  <p className="text-red-400/80 text-sm font-bold">Limites de segurança excedidos. Sistema bloqueado.</p>
                </div>
              </div>
              <button 
                onClick={() => {
                   setState(s => ({...s, alarmeGlobalAtivo: false}));
                   triggerLog('INFO', 'Alarme resetado pelo supervisor.');
                }}
                className="bg-red-600 text-white px-8 py-3 rounded-xl font-black text-xs uppercase hover:bg-red-500 shadow-xl transition-all"
              >Resetar & Reativar</button>
            </div>
          )}

          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-8">
            <TelemetryNode 
              label="Arc Voltage" value={currentData.tensao.toFixed(1)} unit="VDC" status={currentData.tensao > 85 ? 'error' : 'normal'} icon={Activity}
            />
            <TelemetryNode 
              label="Arc Current" value={currentData.corrente.toFixed(0)} unit="AMPS" status={currentData.corrente > 400 ? 'warning' : 'normal'} icon={Zap}
            />
            <TelemetryNode 
              label="Core Temp" value={currentData.temperatura.toFixed(1)} unit="°C" status={currentData.temperatura > 75 ? 'error' : 'normal'} icon={Thermometer}
            />
            <TelemetryNode 
              label="Wire Feed Speed" value={currentData.rpm} unit="RPM" status={'normal'} icon={Wind}
            />
          </div>

          <div className="grid grid-cols-12 gap-10">
            {/* GRÁFICO TÉCNICO */}
            <div className="col-span-12 lg:col-span-8 scada-panel rounded-2xl p-8 flex flex-col">
              <div className="flex items-center justify-between mb-10">
                <div className="flex items-center gap-4">
                  <TrendingUp className="text-blue-500 w-6 h-6" />
                  <div>
                    <h3 className="text-xs font-black uppercase tracking-[0.2em] text-slate-400">Process Waveform Analysis</h3>
                    <p className="text-[10px] text-slate-600 font-bold uppercase mt-1">Estabilidade dinâmica do arco elétrico</p>
                  </div>
                </div>
                <div className="flex items-center gap-6 text-[10px] font-black text-slate-500 uppercase">
                  <span className="flex items-center gap-2"><div className="w-4 h-1 bg-blue-500 rounded-full"></div> Voltagem</span>
                  <span className="flex items-center gap-2"><div className="w-4 h-1 bg-amber-500 rounded-full"></div> Amperagem</span>
                </div>
              </div>
              <div className="flex-1 min-h-[350px]">
                <ResponsiveContainer width="100%" height="100%">
                  <AreaChart data={dataHistory}>
                    <defs>
                      <linearGradient id="colorV" x1="0" y1="0" x2="0" y2="1">
                        <stop offset="5%" stopColor="#3b82f6" stopOpacity={0.2}/>
                        <stop offset="95%" stopColor="#3b82f6" stopOpacity={0}/>
                      </linearGradient>
                      <linearGradient id="colorI" x1="0" y1="0" x2="0" y2="1">
                        <stop offset="5%" stopColor="#f59e0b" stopOpacity={0.2}/>
                        <stop offset="95%" stopColor="#f59e0b" stopOpacity={0}/>
                      </linearGradient>
                    </defs>
                    <CartesianGrid strokeDasharray="2 2" stroke="#1f2937" vertical={false} />
                    <XAxis dataKey="timestamp" hide />
                    <YAxis stroke="#4b5563" fontSize={10} fontStyle="bold" axisLine={false} tickLine={false} />
                    <Tooltip 
                      contentStyle={{ backgroundColor: '#11141b', borderColor: '#334155', borderRadius: '12px', fontSize: '10px', fontWeight: 'bold' }}
                      itemStyle={{ color: '#fff' }}
                    />
                    <Area type="stepAfter" dataKey="tensao" stroke="#3b82f6" strokeWidth={3} fillOpacity={1} fill="url(#colorV)" animationDuration={300} />
                    <Area type="stepAfter" dataKey="corrente" stroke="#f59e0b" strokeWidth={3} fillOpacity={1} fill="url(#colorI)" animationDuration={300} />
                  </AreaChart>
                </ResponsiveContainer>
              </div>
            </div>

            {/* AI DIAGNOSTICS & EVENT LOGGER */}
            <div className="col-span-12 lg:col-span-4 flex flex-col gap-8">
               <div className="scada-panel rounded-2xl p-8 border-t-4 border-blue-600 flex flex-col gap-5 shadow-lg">
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-3">
                      <div className="p-1.5 bg-blue-600/20 rounded-lg">
                        <Cpu className="w-4 h-4 text-blue-500" />
                      </div>
                      <h3 className="text-xs font-black uppercase tracking-widest text-white">AI Technical Insight</h3>
                    </div>
                    <button 
                      onClick={handleAiDiagnostic}
                      disabled={isAiLoading || (!serialConnected && !state.relesSistemaLigado)}
                      className="p-2 hover:bg-slate-800 rounded-lg text-blue-400 transition-all disabled:opacity-20"
                    >
                      <Sliders className={`w-5 h-5 ${isAiLoading ? 'animate-spin' : ''}`} />
                    </button>
                  </div>
                  <div className="bg-black/40 rounded-xl p-5 text-[11px] font-bold leading-relaxed min-h-[160px] max-h-[220px] overflow-y-auto text-slate-400 border border-slate-800/50 shadow-inner">
                     {isAiLoading ? (
                       <div className="flex flex-col items-center justify-center h-full gap-3">
                          <div className="w-2 h-2 bg-blue-500 rounded-full animate-ping"></div>
                          <span className="uppercase text-[9px] tracking-widest">Calculando Parâmetros...</span>
                       </div>
                     ) : aiInsight ? (
                       <p className="whitespace-pre-wrap">{aiInsight}</p>
                     ) : (
                       <div className="flex flex-col items-center justify-center h-full text-center opacity-30">
                          <Info className="w-8 h-8 mb-2" />
                          <p>Conecte o USB e inicie o arco para análise preditiva.</p>
                       </div>
                     )}
                  </div>
               </div>

               <div className="scada-panel rounded-2xl flex-1 overflow-hidden flex flex-col shadow-lg border border-slate-800">
                  <div className="bg-[#1a1d24] px-6 py-3 flex items-center justify-between border-b border-slate-800">
                     <div className="flex items-center gap-2">
                        <Terminal className="w-3 h-3 text-slate-500" />
                        <span className="text-[10px] font-black uppercase text-slate-400 tracking-widest">Master Event Logger</span>
                     </div>
                     <div className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse shadow-[0_0_5px_#10b981]"></div>
                  </div>
                  <div className="flex-1 overflow-y-auto p-6 space-y-3 font-mono text-[10px]">
                    {eventLog.map((log, i) => (
                      <div key={i} className={`flex gap-4 items-start ${log.type === 'CRIT' ? 'text-red-400 font-bold bg-red-500/5 p-1 rounded' : log.type === 'WARN' ? 'text-amber-400' : 'text-slate-500'}`}>
                        <span className="opacity-30 whitespace-nowrap">[{log.time}]</span>
                        <ChevronRight className={`w-3 h-3 mt-0.5 ${log.type === 'CRIT' ? 'animate-bounce' : ''}`} />
                        <span className="text-slate-300 tracking-tighter uppercase">{log.msg}</span>
                      </div>
                    ))}
                    {eventLog.length === 0 && (
                      <div className="flex flex-col items-center justify-center h-full opacity-10">
                        <Database className="w-10 h-10 mb-2" />
                        <span className="uppercase text-[8px] font-black tracking-[0.3em]">Buffer Empty</span>
                      </div>
                    )}
                  </div>
               </div>
            </div>
          </div>
        </main>
      </div>

      <footer className="h-12 bg-[#11141b] border-t border-slate-800 px-8 flex items-center justify-between text-[10px] font-black uppercase tracking-widest text-slate-500 shadow-[0_-10px_30px_rgba(0,0,0,0.5)]">
        <div className="flex items-center gap-10">
           <div className="flex items-center gap-2">
             <HardDrive className="w-3 h-3" />
             NODE_01: <span className={serialConnected ? "text-emerald-500" : "text-amber-500"}>{serialConnected ? 'HARDWARE_LINK' : 'EMULATOR'}</span>
           </div>
           <div className="flex items-center gap-2">
             <Network className="w-3 h-3" />
             SPEED: 115.2 KBPS
           </div>
        </div>
        <div className="flex items-center gap-6">
           <div className="flex items-center gap-2">
              <User className="w-3 h-3" />
              OPERATOR: <span className="text-blue-500">ROOT_ADMIN</span>
           </div>
           <span className="text-slate-800">|</span>
           <span className="font-light italic text-slate-600 lowercase tracking-normal">© 2025 weldmaster industrial solutions</span>
        </div>
      </footer>
    </div>
  );
};

export default App;
