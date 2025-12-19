
export interface WeldingData {
  tensao: number;
  corrente: number;
  temperatura: number;
  fluxo: number;
  rpm: number;
  tensaoPico: number;
  correntePico: number;
  timestamp: number;
}

export interface SystemState {
  relesSistemaLigado: boolean;
  relesModoSMAW: boolean;
  alarmeGlobalAtivo: boolean;
  emergenciaAtiva: boolean;
}

export interface CalibrationData {
  calibIZERO: number;
  calibISPAN: number;
  calibVSPAN: number;
}

export interface Alarme {
  ativo: boolean;
  reconhecido: boolean;
  nome: string;
  timestamp: number;
}
