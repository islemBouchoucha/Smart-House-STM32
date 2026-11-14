# Protocole UART STM32 ↔ ESP32

Liaison série 115200 bauds, 8N1, 3,3 V. Messages texte, une ligne par message, terminée par `\n`.

## STM32 → ESP32 (toutes les 500 ms)

```
STATE;T=25.3;GAS=120;PIR=0;LIGHT=1;FAN=0;FANMODE=2;DOOR=0;ALARM=0;MUTE=0
```

| Clé | Signification |
|---|---|
| T | Température en °C (LM35) |
| GAS | Valeur ADC brute 0-4095 (MQ-2) |
| PIR | 1 = présence détectée |
| LIGHT | 1 = lumière allumée |
| FAN | 1 = ventilateur actif |
| FANMODE | 0 = OFF, 1 = ON, 2 = AUTO |
| DOOR | 1 = porte ouverte |
| ALARM | 1 = seuil de gaz dépassé |
| MUTE | 1 = buzzer coupé |

## ESP32 → STM32

```
CMD;LIGHT=1        lumière ON/OFF (1/0)
CMD;FANMODE=2      ventilateur 0 OFF / 1 ON / 2 AUTO
CMD;DOOR=1         porte ouverte/fermée (1/0)
CMD;MUTE=1         coupe / réactive le buzzer (1/0)
```

## API HTTP de l'ESP32

| Route | Rôle |
|---|---|
| `GET /` | Page web de contrôle |
| `GET /api/state` | Dernier état en JSON (`online` = false si le STM32 ne répond plus depuis 3 s) |
| `GET /api/cmd?c=LIGHT&v=1` | Envoie une commande au STM32 (clés autorisées : LIGHT, FANMODE, DOOR, MUTE) |
