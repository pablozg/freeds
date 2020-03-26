/*
  pwm.ino - FreeDs pwm functions
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32

  Based in opends+ (https://github.com/iqas/derivador)
  
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

  DEBUGLN("\r\nPWMCONTROL()");

  // Check pwm_output

  if (inverter.wgrid_control != inverter.wgrid) // En caso de recepción de lectura, actualizamos el valor de invert_wgrid_control
  {
    inverter.wgrid_control = inverter.wgrid;
    Error.LecturaDatos = false;
    timers.ErrorLecturaDatos = millis();
  }

  if (((millis() - timers.ErrorLecturaDatos) > config.maxErrorTime))
  {
    Error.LecturaDatos = true;
    timers.ErrorLecturaDatos = millis();
    memset(&inverter, 0, sizeof(inverter));
    memset(&meter, 0, sizeof(meter));
    INFOLN(F("PWM: Apagando PWM por superar el tiempo máximo en la recepción de datos del vertido a Red"));
  }

  if (!config.P01_on || (!config.pwm_man && (Error.LecturaDatos || Error.ConexionInversor)))
  {
    down_pwm(true);
  }

  if (config.pwm_man && config.P01_on)
  {
    uint16_t maxPwm = (1023 * config.manualControlPWM) / 100;

    relay_control_man(false);

    if (invert_pwm <= maxPwm)
    {
      if (invert_pwm <= slowPwm) // hasta el 20 % subida lenta
        invert_pwm += 20;
      else if (invert_pwm > slowPwm)
        invert_pwm += 50;

      invert_pwm = constrain(invert_pwm, 0, maxPwm);
      if (invert_pwm != last_invert_pwm)
      {
        DEBUGLN(F("PWM MANUAL: SUBIENDO POTENCIA"));
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
        DEBUGLN(F("PWM MANUAL: BAJANDO POTENCIA"));
        last_invert_pwm = invert_pwm;
      }
      ledcWrite(2, invert_pwm);
      dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
    }
  }

  // Control automatico del pwm
  if (config.P01_on && !config.pwm_man && !Error.LecturaDatos && inverter.wgrid != 0)
  {
    if (inverter.wgrid > config.pwm_min)
    {
      if (inverter.wgrid > config.pwm_max && invert_pwm < slowPwm)
        invert_pwm += 10;
      else if (inverter.wgrid > config.pwm_max && invert_pwm > slowPwm)
        invert_pwm += 25;

      invert_pwm = constrain(invert_pwm, 0, 1023); // Limitamos el valor
      if (invert_pwm != last_invert_pwm)
      {
        DEBUGLN(F("PWM: SUBIENDO POTENCIA"));
        last_invert_pwm = invert_pwm;
      }
      ledcWrite(2, invert_pwm); // Write new pwm value
      dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
    }

    else if (inverter.wgrid < config.pwm_max)
    {
      if (inverter.wgrid < config.pwm_max && invert_pwm > slowPwm)
        invert_pwm -= 35; // 50
      else if (inverter.wgrid < config.pwm_max && invert_pwm < slowPwm)
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
      invert_pwm = constrain(invert_pwm, 0, 1023); // Limitamos el valor
      if (invert_pwm != last_invert_pwm)
      {
        DEBUGLN(F("PWM: BAJANDO POTENCIA"));
        last_invert_pwm = invert_pwm;
      }
      ledcWrite(2, invert_pwm); // Write new pwm value
      dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
    }
  }

  calcPwmProgressBar();

  ///////////////////// SALIDAS POR PORCENTAJE /////////////////////////

  // Start at defined pwm value

  if (!config.pwm_man)
  {
    if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && (progressbar >= config.R01_min))
    {
      // Start relay1
      digitalWrite(PIN_RL1, HIGH);
      INFOLN("Encendido por % Salida 1");
      Flags.RelayTurnOn = false;
      Flags.Relay01Auto = true;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL2) && Flags.RelayTurnOn && (progressbar >= config.R02_min))
    {
      // Start relay2
      digitalWrite(PIN_RL2, HIGH);
      INFOLN("Encendido por % Salida 2");
      Flags.RelayTurnOn = false;
      Flags.Relay02Auto = true;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL3) && Flags.RelayTurnOn && (progressbar >= config.R03_min))
    {
      // Start relay3
      digitalWrite(PIN_RL3, HIGH);
      INFOLN("Encendido por % Salida 3");
      Flags.RelayTurnOn = false;
      Flags.Relay03Auto = true;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    }

    if (!digitalRead(PIN_RL4) && Flags.RelayTurnOn && (progressbar >= config.R04_min))
    {
      // Start relay4
      digitalWrite(PIN_RL4, HIGH);
      INFOLN("Encendido por % Salida 4");
      Flags.RelayTurnOn = false;
      Flags.Relay04Auto = true;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
    }
  }

  // Stop at defined porcent of power

  if (!Flags.RelayTurnOff)
  {
    if (!(Flags.Relay04Man || config.R04_man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && (progressbar < config.R04_min) && config.R04_min != 999)
    {
      digitalWrite(PIN_RL4, LOW);
      INFOLN("Apagado por % Salida 4");
      Flags.RelayTurnOff = true;
      Flags.Relay04Auto = false;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      {
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay03Man || config.R03_man) && digitalRead(PIN_RL3) && !Flags.RelayTurnOff && (progressbar < config.R03_min) && config.R03_min != 999)
    {
      digitalWrite(PIN_RL3, LOW);
      INFOLN("Apagado por % Salida 3");
      Flags.RelayTurnOff = true;
      Flags.Relay03Auto = false;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      {
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay02Man || config.R02_man) && digitalRead(PIN_RL2) && !Flags.RelayTurnOff && (progressbar < config.R02_min) && config.R02_min != 999)
    {
      digitalWrite(PIN_RL2, LOW);
      INFOLN("Apagado por % Salida 2");
      Flags.RelayTurnOff = true;
      Flags.Relay02Auto = false;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      {
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
      }
    }
    if (!(Flags.Relay01Man || config.R01_man) && digitalRead(PIN_RL1) && !Flags.RelayTurnOff && (progressbar < config.R01_min) && config.R01_min != 999)
    {
      digitalWrite(PIN_RL1, LOW);
      INFOLN("Apagado por % Salida 1");
      Flags.RelayTurnOff = true;
      Flags.Relay01Auto = false;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      {
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
      }
    }
  }

  ///////////////////// SALIDAS POR POTENCIA /////////////////////////

  // Start at defined power on

  if (!config.pwm_man && config.P01_on && (progressbar >= config.autoControlPWM))
  {
    if (!digitalRead(PIN_RL1) && Flags.RelayTurnOn && config.R01_min == 999 && (inverter.wgrid > config.R01_poton))
    {
      // Start relay1
      digitalWrite(PIN_RL1, HIGH);
      INFOLN("Encendido por W Salida 1");
      Flags.Relay01Auto = true;
      Flags.RelayTurnOn = false;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL2) && Flags.RelayTurnOn && config.R02_min == 999 && (inverter.wgrid > config.R02_poton))
    {
      // Start relay2
      digitalWrite(PIN_RL2, HIGH);
      INFOLN("Encendido por W Salida 2");
      Flags.Relay02Auto = true;
      Flags.RelayTurnOn = false;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL3) && Flags.RelayTurnOn && config.R03_min == 999 && (inverter.wgrid > config.R03_poton))
    {
      // Start relay3
      digitalWrite(PIN_RL3, HIGH);
      INFOLN("Encendido por W Salida 3");
      Flags.Relay03Auto = true;
      Flags.RelayTurnOn = false;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    }
    if (!digitalRead(PIN_RL4) && Flags.RelayTurnOn && config.R04_min == 999 && (inverter.wgrid > config.R04_poton))
    {
      // Start relay4
      digitalWrite(PIN_RL4, HIGH);
      INFOLN("Encendido por W Salida 4");
      Flags.Relay04Auto = true;
      Flags.RelayTurnOn = false;
      if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
        publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
    }
  }

  // Stop at defined power off

  if (!(Flags.Relay04Man || config.R04_man) && digitalRead(PIN_RL4) && !Flags.RelayTurnOff && config.R04_min == 999 && (inverter.wgrid < config.R04_potoff))
  {
    // Start relay4
    digitalWrite(PIN_RL4, LOW);
    INFOLN("Apagado por W Salida 4");
    Flags.Relay04Auto = false;
    Flags.RelayTurnOff = true;
    if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");
  }

  if (!(Flags.Relay03Man || config.R03_man) && digitalRead(PIN_RL3) && !Flags.RelayTurnOff && config.R03_min == 999 && (inverter.wgrid < config.R03_potoff))
  {
    // Start relay3
    digitalWrite(PIN_RL3, LOW);
    INFOLN("Apagado por W Salida 3");
    Flags.Relay03Auto = false;
    Flags.RelayTurnOff = true;
    if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
  }

  if (!(Flags.Relay02Man || config.R02_man) && digitalRead(PIN_RL2) && !Flags.RelayTurnOff && config.R02_min == 999 && (inverter.wgrid < config.R02_potoff))
  {
    // Start relay2
    digitalWrite(PIN_RL2, LOW);
    INFOLN("Apagado por W Salida 2");
    Flags.Relay02Auto = false;
    Flags.RelayTurnOff = true;
    if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
      publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
  }

  if (!(Flags.Relay01Man || config.R01_man) && digitalRead(PIN_RL1) && !Flags.RelayTurnOff && config.R01_min == 999 && (inverter.wgrid < config.R01_potoff))
  {
    // Start relay1
    digitalWrite(PIN_RL1, LOW);
    INFOLN("Apagado por W Salida 1");
    Flags.Relay01Auto = false;
    Flags.RelayTurnOff = true;
    if (config.mqtt && !Error.ConexionMqtt && config.wversion != 0)
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
  DEBUG("\r\nPWM VALUE: % ");
  DEBUGLN(pro);
  DEBUG("\r\nPWM VALUE: ");
  DEBUGLN(invert_pwm);
}

void relay_control_man(boolean forceOFF)
{

  DEBUGLN("\r\nrelay_control_man()");

  // if (!(Flags.Relay01Man || config.R01_man) && !Flags.Relay01Auto)
  //   digitalWrite(PIN_RL1, LOW);
  // if (!(Flags.Relay02Man || config.R02_man) && !Flags.Relay02Auto)
  //   digitalWrite(PIN_RL2, LOW);
  // if (!(Flags.Relay03Man || config.R03_man) && !Flags.Relay03Auto)
  //   digitalWrite(PIN_RL3, LOW);
  // if (!(Flags.Relay04Man || config.R04_man) && !Flags.Relay04Auto)
  //   digitalWrite(PIN_RL4, LOW);

  // Se activan las salidas seleccionadas manualmente si no lo están ya
  if ((Flags.Relay01Man || config.R01_man) && !digitalRead(PIN_RL1))
    digitalWrite(PIN_RL1, HIGH);
  if ((Flags.Relay02Man || config.R02_man) && !digitalRead(PIN_RL2))
    digitalWrite(PIN_RL2, HIGH);
  if ((Flags.Relay03Man || config.R03_man) && !digitalRead(PIN_RL3))
    digitalWrite(PIN_RL3, HIGH);
  if ((Flags.Relay04Man || config.R04_man) && !digitalRead(PIN_RL4))
    digitalWrite(PIN_RL4, HIGH);

  // Se fuerza el apagado de todas las salidas
  if (forceOFF || !config.P01_on)
  {
    digitalWrite(PIN_RL1, LOW);
    digitalWrite(PIN_RL2, LOW);
    digitalWrite(PIN_RL3, LOW);
    digitalWrite(PIN_RL4, LOW);
    INFOLN("Relays Forced Down");
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
}

void down_pwm(boolean softDown)
{
  if (softDown == true)
  {
    invertPwmDown();
    ledcWrite(2, invert_pwm); // Write new pwm value
    dac_output_voltage(DAC_CHANNEL_2, constrain((invert_pwm / 4), 0, 255));
  }
  else
  {
    ledcWrite(2, 0); // Hard shutdown
    dac_output_disable(DAC_CHANNEL_2);
    INFOLN("PWM DEACTIVATED");
  }
  if (invert_pwm != last_invert_pwm)
  {
    DEBUGLN(F("PWM: DEACTIVATING PWM"));
    last_invert_pwm = invert_pwm;
  }

  relay_control_man(!softDown);
  calcPwmProgressBar();
}
