/*
  pwm.ino - FreeDs pwm functions
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32

  Inspired in opends+ (https://github.com/iqas/derivador)
  
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


#define slowPwm 204 // 20%

void pwmControl()
{

  if (config.flags.moreDebug) { INFOV("PWMCONTROL()\n"); }

  // Check pwm_output

  if (inverter.wgrid_control != inverter.wgrid) // En caso de recepción de lectura, actualizamos el valor de invert_wgrid_control
  {
    inverter.wgrid_control = inverter.wgrid;
    Error.VariacionDatos = false;
    timers.ErrorVariacionDatos = millis();
  }

  if (((millis() - timers.ErrorVariacionDatos) > config.maxErrorTime) && !Error.VariacionDatos)
  {
    INFOV(PSTR("PWM: Apagando PWM por superar el tiempo máximo en la recepción de los datos del consumo de Red\n"));
    Error.VariacionDatos = true;
    memset(&inverter, 0, sizeof(inverter));
    memset(&meter, 0, sizeof(meter));
  }

  if (!config.flags.pwmEnabled || (!config.flags.pwmMan && (Error.VariacionDatos || Error.RecepcionDatos)))
  {
    down_pwm(true);
  }

  // Control manual del pwm
  if ((config.flags.pwmMan || Flags.pwmManAuto) && config.flags.pwmEnabled && Flags.pwmIsWorking)
  {
    uint16_t maxPwm = (1023 * config.manualControlPWM) / 100;

    relay_control_man(false);

    if (invert_pwm <= maxPwm)
    {
      #ifdef PWMCHINOMALO
        if (invert_pwm < 200) { invert_pwm = 200; } // Solución Fernando
      #endif
      if (invert_pwm <= slowPwm) // hasta el 20 % subida lenta
        invert_pwm += 20;
      else if (invert_pwm > slowPwm)
        invert_pwm += 50;

      invert_pwm = constrain(invert_pwm, 0, maxPwm);
      if (invert_pwm != last_invert_pwm)
      {
        if (config.flags.debug) { INFOV(PSTR("PWM MANUAL: SUBIENDO POTENCIA\n")); }
        last_invert_pwm = invert_pwm;
      }
      ledcWrite(2, invert_pwm);
      dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
    }
    else if (invert_pwm > maxPwm)
    {
      invertPwmDown();
      invert_pwm = constrain(invert_pwm, 0, 1023);
      if (invert_pwm != last_invert_pwm)
      {
        if (config.flags.debug) { INFOV(PSTR("PWM MANUAL: BAJANDO POTENCIA\n")); }
        last_invert_pwm = invert_pwm;
      }
      ledcWrite(2, invert_pwm);
      dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
    }
  }

  // Control automatico del pwm
  if (config.flags.pwmEnabled && !config.flags.pwmMan && !Flags.pwmManAuto && !Error.VariacionDatos && Flags.pwmIsWorking)
  {
    if (inverter.wgrid > config.pwmMin && inverter.batteryWatts >= 0)
    {
      if (inverter.wgrid > config.pwmMax && invert_pwm <= slowPwm)
      {
        #ifdef PWMCHINOMALO
          if (invert_pwm < 200) { invert_pwm = 200; } // Solución Fernando
        #endif
        invert_pwm += 10;
      }
      else if (inverter.wgrid > config.pwmMax && invert_pwm > slowPwm)
        invert_pwm += 25;

      invert_pwm = constrain(invert_pwm, 0, 1023); // Limitamos el valor
      if (invert_pwm != last_invert_pwm)
      {
        if (config.flags.debug) { INFOV(PSTR("PWM: SUBIENDO POTENCIA\n")); }
        last_invert_pwm = invert_pwm;
      }
      ledcWrite(2, invert_pwm); // Write new pwm value
      dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
    }
    else if (inverter.wgrid < config.pwmMax || inverter.batteryWatts < 0)
    {
      if ((inverter.wgrid < config.pwmMax || inverter.batteryWatts < 0) && invert_pwm > slowPwm)
        invert_pwm -= 35; // 50
      else if ((inverter.wgrid < config.pwmMax || inverter.batteryWatts < 0) && invert_pwm <= slowPwm)
      {
        if (invert_pwm > 20)
          invert_pwm -= 20;
        else if (invert_pwm > 15)
          invert_pwm -= 15;
        else if (invert_pwm > 5)
          invert_pwm -= 5;
        else if (invert_pwm >= 1)
          invert_pwm--;

        #ifdef PWMCHINOMALO
          if (invert_pwm <= 200) { invert_pwm = 0; } // Solución Fernando
        #endif
      }
      invert_pwm = constrain(invert_pwm, 0, 1023); // Limitamos el valor
      if (invert_pwm != last_invert_pwm)
      {
        if (config.flags.debug) { INFOV(PSTR("PWM: BAJANDO POTENCIA\n")); }
        last_invert_pwm = invert_pwm;
      }
      ledcWrite(2, invert_pwm); // Write new pwm value
      dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
    }
  }

  calcPwmProgressBar();

  ///////////////////// SALIDAS POR PORCENTAJE /////////////////////////

  // Start at defined pwm value

  if (!config.flags.pwmMan && config.flags.pwmEnabled)
  {
    if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && (progressbar >= config.R01Min))
    {
      // Start relay1
      digitalWrite(PIN_RL1, HIGH);
      INFOV("Encendido por %% Salida 1\n");
      Flags.RelayTurnOn = false;
      Flags.Relay01Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL2) && Flags.RelayTurnOn && (progressbar >= config.R02Min))
    {
      // Start relay2
      digitalWrite(PIN_RL2, HIGH);
      INFOV("Encendido por %% Salida 2\n");
      Flags.RelayTurnOn = false;
      Flags.Relay02Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL3) && Flags.RelayTurnOn && (progressbar >= config.R03Min))
    {
      // Start relay3
      digitalWrite(PIN_RL3, HIGH);
      INFOV("Encendido por %% Salida 3\n");
      Flags.RelayTurnOn = false;
      Flags.Relay03Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL4) && Flags.RelayTurnOn && (progressbar >= config.R04Min))
    {
      // Start relay4
      digitalWrite(PIN_RL4, HIGH);
      INFOV("Encendido por %% Salida 4\n");
      Flags.RelayTurnOn = false;
      Flags.Relay04Auto = true;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
    }
  }

  // Stop at defined porcent of power

  if (!Flags.RelayTurnOff)
  {
    if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && (progressbar <= ((config.R04Min - 10) < 0 ? 0 : (config.R04Min - 10))) && config.R04Min != 999)
    {
      digitalWrite(PIN_RL4, LOW);
      INFOV("Apagado por %% Salida 4\n");
      Flags.RelayTurnOff = true;
      Flags.Relay04Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      {
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay03Man || config.relaysFlags.R03Man) && digitalRead(PIN_RL3) && !Flags.RelayTurnOff && (progressbar <= ((config.R03Min - 10) < 0 ? 0 : (config.R03Min - 10))) && config.R03Min != 999)
    {
      digitalWrite(PIN_RL3, LOW);
      INFOV("Apagado por %% Salida 3\n");
      Flags.RelayTurnOff = true;
      Flags.Relay03Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      {
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay02Man || config.relaysFlags.R02Man) && digitalRead(PIN_RL2) && !Flags.RelayTurnOff && (progressbar <= ((config.R02Min - 10) < 0 ? 0 : (config.R02Min - 10))) && config.R02Min != 999)
    {
      digitalWrite(PIN_RL2, LOW);
      INFOV("Apagado por %% Salida 2\n");
      Flags.RelayTurnOff = true;
      Flags.Relay02Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      {
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay01Man || config.relaysFlags.R01Man) && digitalRead(PIN_RL1) && !Flags.RelayTurnOff && (progressbar <= ((config.R01Min - 10) < 0 ? 0 : (config.R01Min - 10))) && config.R01Min != 999)
    {
      digitalWrite(PIN_RL1, LOW);
      INFOV("Apagado por %% Salida 1\n");
      Flags.RelayTurnOff = true;
      Flags.Relay01Auto = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      {
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
      }
    }
  }

  ///////////////////// SALIDAS POR POTENCIA /////////////////////////

  // Start at defined power on

  if (!config.flags.pwmMan && config.flags.pwmEnabled && (progressbar >= config.autoControlPWM))
  {
    if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && config.R01Min == 999 && (inverter.wgrid > config.R01PotOn))
    {
      // Start relay1
      digitalWrite(PIN_RL1, HIGH);
      INFOV("Encendido por W Salida 1\n");
      Flags.Relay01Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL2) && Flags.RelayTurnOn && config.R02Min == 999 && (inverter.wgrid > config.R02PotOn))
    {
      // Start relay2
      digitalWrite(PIN_RL2, HIGH);
      INFOV("Encendido por W Salida 2\n");
      Flags.Relay02Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL3) && Flags.RelayTurnOn && config.R03Min == 999 && (inverter.wgrid > config.R03PotOn))
    {
      // Start relay3
      digitalWrite(PIN_RL3, HIGH);
      INFOV("Encendido por W Salida 3\n");
      Flags.Relay03Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL4) && Flags.RelayTurnOn && config.R04Min == 999 && (inverter.wgrid > config.R04PotOn))
    {
      // Start relay4
      digitalWrite(PIN_RL4, HIGH);
      INFOV("Encendido por W Salida 4\n");
      Flags.Relay04Auto = true;
      Flags.RelayTurnOn = false;
      if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
    }
  }

  // Stop at defined power off

  if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && config.R04Min == 999 && (inverter.wgrid < config.R04PotOff))
  {
    // Start relay4
    digitalWrite(PIN_RL4, LOW);
    INFOV("Apagado por W Salida 4\n");
    Flags.Relay04Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
  }

  if (!(Flags.Relay03Man || config.relaysFlags.R03Man) && digitalRead(PIN_RL3) && !Flags.RelayTurnOff && config.R03Min == 999 && (inverter.wgrid < config.R03PotOff))
  {
    // Start relay3
    digitalWrite(PIN_RL3, LOW);
    INFOV("Apagado por W Salida 3\n");
    Flags.Relay03Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
  }

  if (!(Flags.Relay02Man || config.relaysFlags.R02Man) && digitalRead(PIN_RL2) && !Flags.RelayTurnOff && config.R02Min == 999 && (inverter.wgrid < config.R02PotOff))
  {
    // Start relay2
    digitalWrite(PIN_RL2, LOW);
    INFOV("Apagado por W Salida 2\n");
    Flags.Relay02Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
  }

  if (!(Flags.Relay01Man || config.relaysFlags.R01Man) && digitalRead(PIN_RL1) && !Flags.RelayTurnOff && config.R01Min == 999 && (inverter.wgrid < config.R01PotOff))
  {
    // Start relay1
    digitalWrite(PIN_RL1, LOW);
    INFOV("Apagado por W Salida 1\n");
    Flags.Relay01Auto = false;
    Flags.RelayTurnOff = true;
    if (config.flags.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
  }

  if (!Flags.RelayTurnOn && xTimerIsTimerActive(relayOnTimer) == pdFALSE)
  {
    xTimerStart(relayOnTimer, 0);
  }

  if (Flags.RelayTurnOff && xTimerIsTimerActive(relayOffTimer) == pdFALSE)
  {
    xTimerStart(relayOffTimer, 0);
  } 
}

void enableRelay(void)
{
  Flags.RelayTurnOn = true;
  xTimerStop(relayOnTimer, 0);
}

void disableRelay(void)
{
  Flags.RelayTurnOff = false;
  xTimerStop(relayOffTimer, 0);
}

void calcPwmProgressBar()
{
  char tmp[7];

  dtostrfd(((invert_pwm * 100) / 1022), 0, tmp);
  pro = String(tmp);
  progressbar = ((invert_pwm * 100) / 1022);

  // Print PWM Values
  // if (config.flags.debug) {
  //   INFOV("PWM Value: %s%%, %lu\n", tmp, invert_pwm);
  // }
}

void relay_control_man(boolean forceOFF)
{

  if (config.flags.moreDebug) { INFOV("relay_control_man()\n"); }

  // if (!(Flags.Relay01Man || config.relaysFlags.R01Man) && !Flags.Relay01Auto)
  //   digitalWrite(PIN_RL1, LOW);
  // if (!(Flags.Relay02Man || config.relaysFlags.R02Man) && !Flags.Relay02Auto)
  //   digitalWrite(PIN_RL2, LOW);
  // if (!(Flags.Relay03Man || config.relaysFlags.R03Man) && !Flags.Relay03Auto)
  //   digitalWrite(PIN_RL3, LOW);
  // if (!(Flags.Relay04Man || config.relaysFlags.R04Man) && !Flags.Relay04Auto)
  //   digitalWrite(PIN_RL4, LOW);

  // Se activan las salidas seleccionadas manualmente si no lo están ya
  if ((Flags.Relay01Man || config.relaysFlags.R01Man) && !digitalRead(PIN_RL1)) { digitalWrite(PIN_RL1, HIGH); INFOV("Relay 1 Forced On\n");}
  if ((Flags.Relay02Man || config.relaysFlags.R02Man) && !digitalRead(PIN_RL2)) { digitalWrite(PIN_RL2, HIGH); INFOV("Relay 2 Forced On\n");}
  if ((Flags.Relay03Man || config.relaysFlags.R03Man) && !digitalRead(PIN_RL3)) { digitalWrite(PIN_RL3, HIGH); INFOV("Relay 3 Forced On\n");}
  if ((Flags.Relay04Man || config.relaysFlags.R04Man) && !digitalRead(PIN_RL4)) { digitalWrite(PIN_RL4, HIGH); INFOV("Relay 4 Forced On\n");}

  // Se fuerza el apagado de todas las salidas
    if (forceOFF || !config.flags.pwmEnabled)
  {
    if (digitalRead(PIN_RL1) && !Flags.Relay01Man && !config.relaysFlags.R01Man) { digitalWrite(PIN_RL1, LOW); INFOV("Relay 1 Forced Off\n"); }
    if (digitalRead(PIN_RL2) && !Flags.Relay02Man && !config.relaysFlags.R02Man) { digitalWrite(PIN_RL2, LOW); INFOV("Relay 2 Forced Off\n"); }
    if (digitalRead(PIN_RL3) && !Flags.Relay03Man && !config.relaysFlags.R03Man) { digitalWrite(PIN_RL3, LOW); INFOV("Relay 3 Forced Off\n"); }
    if (digitalRead(PIN_RL4) && !Flags.Relay04Man && !config.relaysFlags.R04Man) { digitalWrite(PIN_RL4, LOW); INFOV("Relay 4 Forced Off\n"); }
  }
}

void invertPwmDown(void)
{
  if (invert_pwm > slowPwm)
    invert_pwm -= 35; // 50
  else if (invert_pwm <= slowPwm)
  {
    if (invert_pwm > 20)
      invert_pwm -= 20;
    else if (invert_pwm > 15)
      invert_pwm -= 15;
    else if (invert_pwm > 5)
      invert_pwm -= 5;
    else if (invert_pwm >= 1)
      invert_pwm--;
  }
  
  #ifdef PWMCHINOMALO
    if (invert_pwm <= 200) { invert_pwm = 0; } // Solución Fernando
  #endif
}

void down_pwm(boolean softDown)
{
  if (softDown == true)
  {
    invertPwmDown();
    ledcWrite(2, invert_pwm); // Write new pwm value
    dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
  } else {
    invert_pwm = 0;
    ledcWrite(2, 0); // Hard shutdown
    dac_output_disable(DAC_CHANNEL_2);
    INFOV("PWM disabled\n");
  }

  if (invert_pwm != last_invert_pwm)
  {
    if (config.flags.debug) { INFOV(PSTR("PWM: disabling PWM\n")); }
    last_invert_pwm = invert_pwm;
  }

  relay_control_man(!softDown);
  calcPwmProgressBar();
}
