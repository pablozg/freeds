/*
  wibeee.ino - Wibeee Support
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32
  
  Copyright (C) 2020 Pablo Zerón (https://github.com/pablozg/freeds)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//String testDocument = "<?xml version=""1.0"" encoding=""ISO-8859-1""?><response><model>WBB</model><webversion>4.4.094</webversion><time>1583401882</time><fase1_vrms>238.13</fase1_vrms><fase1_irms>2.36</fase1_irms><fase1_p_aparent>561.93</fase1_p_aparent><fase1_p_activa>207.47</fase1_p_activa><fase1_p_reactiva_ind>522.23</fase1_p_reactiva_ind><fase1_p_reactiva_cap>0.00</fase1_p_reactiva_cap><fase1_frecuencia>50.15</fase1_frecuencia><fase1_factor_potencia>0.369</fase1_factor_potencia><fase1_energia_activa>4082176</fase1_energia_activa><fase1_energia_reactiva_ind>13504</fase1_energia_reactiva_ind><fase1_energia_reactiva_cap>848640</fase1_energia_reactiva_cap><fase1_angle>0.00</fase1_angle><fase1_thd_total>0.00</fase1_thd_total><fase1_thd_fund>0.00</fase1_thd_fund><fase1_thd_ar3>0.00</fase1_thd_ar3><fase1_thd_ar5>0.00</fase1_thd_ar5><fase1_thd_ar7>0.00</fase1_thd_ar7><fase1_thd_ar9>0.00</fase1_thd_ar9><fase1_thd_tot_V>0.00</fase1_thd_tot_V><fase1_thd_fun_V>237.80</fase1_thd_fun_V><fase1_thd_ar3_V>0.00</fase1_thd_ar3_V><fase1_thd_ar5_V>0.00</fase1_thd_ar5_V><fase1_thd_ar7_V>0.00</fase1_thd_ar7_V><fase1_thd_ar9_V>0.00</fase1_thd_ar9_V><fase2_vrms>234.22</fase2_vrms><fase2_irms>2.51</fase2_irms><fase2_p_aparent>587.61</fase2_p_aparent><fase2_p_activa>574.96</fase2_p_activa><fase2_p_reactiva_ind>121.31</fase2_p_reactiva_ind><fase2_p_reactiva_cap>0.00</fase2_p_reactiva_cap><fase2_frecuencia>50.46</fase2_frecuencia><fase2_factor_potencia>0.978</fase2_factor_potencia><fase2_energia_activa>164306944</fase2_energia_activa><fase2_energia_reactiva_ind>64</fase2_energia_reactiva_ind><fase2_energia_reactiva_cap>444096</fase2_energia_reactiva_cap><fase2_angle>0.00</fase2_angle><fase2_thd_total>8.30</fase2_thd_total><fase2_thd_fund>2.50</fase2_thd_fund><fase2_thd_ar3>0.00</fase2_thd_ar3><fase2_thd_ar5>0.00</fase2_thd_ar5><fase2_thd_ar7>0.00</fase2_thd_ar7><fase2_thd_ar9>0.20</fase2_thd_ar9><fase2_thd_tot_V>0.00</fase2_thd_tot_V><fase2_thd_fun_V>237.80</fase2_thd_fun_V><fase2_thd_ar3_V>0.00</fase2_thd_ar3_V><fase2_thd_ar5_V>0.00</fase2_thd_ar5_V><fase2_thd_ar7_V>0.00</fase2_thd_ar7_V><fase2_thd_ar9_V>0.00</fase2_thd_ar9_V><fase3_vrms>234.22</fase3_vrms><fase3_irms>2.15</fase3_irms><fase3_p_aparent>503.72</fase3_p_aparent><fase3_p_activa>167.52</fase3_p_activa><fase3_p_reactiva_ind>475.05</fase3_p_reactiva_ind><fase3_p_reactiva_cap>0.00</fase3_p_reactiva_cap><fase3_frecuencia>50.46</fase3_frecuencia><fase3_factor_potencia>0.333</fase3_factor_potencia><fase3_energia_activa>1058816</fase3_energia_activa><fase3_energia_reactiva_ind>51584</fase3_energia_reactiva_ind><fase3_energia_reactiva_cap>122880</fase3_energia_reactiva_cap><fase3_angle>0.00</fase3_angle><fase3_thd_total>118.90</fase3_thd_total><fase3_thd_fund>1.70</fase3_thd_fund><fase3_thd_ar3>1.40</fase3_thd_ar3><fase3_thd_ar5>1.10</fase3_thd_ar5><fase3_thd_ar7>0.70</fase3_thd_ar7><fase3_thd_ar9>0.60</fase3_thd_ar9><fase3_thd_tot_V>0.00</fase3_thd_tot_V><fase3_thd_fun_V>237.80</fase3_thd_fun_V><fase3_thd_ar3_V>0.00</fase3_thd_ar3_V><fase3_thd_ar5_V>0.00</fase3_thd_ar5_V><fase3_thd_ar7_V>0.00</fase3_thd_ar7_V><fase3_thd_ar9_V>0.00</fase3_thd_ar9_V><fase4_vrms>234.22</fase4_vrms><fase4_irms>13.49</fase4_irms><fase4_p_aparent>3160.64</fase4_p_aparent><fase4_p_activa>2802.24</fase4_p_activa><fase4_p_reactiva_ind>397.92</fase4_p_reactiva_ind><fase4_p_reactiva_cap>0.00</fase4_p_reactiva_cap><fase4_frecuencia>50.46</fase4_frecuencia><fase4_factor_potencia>0.887</fase4_factor_potencia><fase4_energia_activa>169447938</fase4_energia_activa><fase4_energia_reactiva_ind>65152</fase4_energia_reactiva_ind><fase4_energia_reactiva_cap>1415616</fase4_energia_reactiva_cap><scale>100</scale><coilStatus>-</coilStatus><ground>0.00</ground></response>";

void parseWibeee(char *xml) {
  
  if (config.flags.debug3) { Serial.printf("Wibee Size: %d, Json: %s\n", strlen(xml), xml); } // No usar INFOV produce stack error
  String response = xml;
  
  // Pinza 1 como lector de Red
  meter.activePower = inverter.wgrid = midString(&response, "<fase1_p_activa>", "</fase1_p_activa>").toFloat();
  meter.voltage =  inverter.gridv = midString(&response, "<fase1_vrms>", "</fase1_vrms>").toFloat();
  meter.current = midString(&response, "<fase1_irms>", "</fase1_irms>").toFloat();
  meter.reactivePower = midString(&response, "<fase1_p_reactiva_ind>", "</fase1_p_reactiva_ind>").toFloat();
  meter.reactivePower += midString(&response, "<fase1_p_reactiva_cap>", "</fase1_p_reactiva_cap>").toFloat();;

  float value;
  value = midString(&response, "<fase1_factor_potencia>", "</fase1_factor_potencia>").toFloat();
  meter.powerFactor = value;
  meter.frequency = midString(&response, "<fase1_frecuencia>", "</fase1_frecuencia>").toFloat();
  // Si powerFactor es negativo está volcando, positivo consumiendo.
  if (value > 0) {
    if (!config.flags.changeGridSign) { meter.activePower *= -1; inverter.wgrid *= -1; meter.reactivePower *= -1; }
  } else {
    meter.powerFactor *= -1;
    if (config.flags.changeGridSign) { meter.activePower *= -1; inverter.wgrid *= -1; meter.reactivePower *= -1; }
  }
 
  meter.importActive =  midString(&response, "<fase1_energia_activa>", "</fase1_energia_activa>").toFloat() / 1000;
  
  // Pinza 2 como lector del Inversor
  inverter.wsolar = midString(&response, "<fase2_p_activa>", "</fase2_p_activa>").toFloat();
  // inverter.gridv = midString(&response, "<fase2_vrms>", "</fase2_vrms>").toFloat();

  Error.RecepcionDatos = false;
  timers.ErrorRecepcionDatos = millis();
}