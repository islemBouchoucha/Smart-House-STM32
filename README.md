# Smart House — Maison intelligente STM32 + ESP32

Système domotique embarqué : un **STM32** gère l'acquisition des capteurs et la commande temps réel des actionneurs, et un **ESP32** assure le Wi-Fi et le contrôle à distance via un serveur web embarqué (C++ / HTML). Les deux cartes communiquent par **UART**.

> Projet académique — ENSI, Université de la Manouba (09/2026).

## Fonctionnalités

- Mesure de la température (LM35), du gaz (MQ-2) et de la présence (PIR)
- Commande de la lumière, du ventilateur (OFF / ON / AUTO avec hystérésis) et de la porte (servomoteur)
- Alarme sonore automatique en cas de fuite de gaz, avec bouton pour couper le buzzer
- Interface web de contrôle, rafraîchie chaque seconde
- Détection de la perte de liaison avec le STM32 (statut « hors ligne »)
- Temps de réponse inférieur à 100 ms, accès 24/7

## Architecture

```
┌──────────────┐   Wi-Fi / HTTP   ┌──────────────┐     UART      ┌──────────────┐
│ Navigateur   │ ◄──────────────► │    ESP32     │ ◄───────────► │    STM32     │
│ (interface)  │                  │ Serveur web  │ commandes /   │ Capteurs     │
└──────────────┘                  └──────────────┘ états capteurs│ Actionneurs  │
                                                                 └──────────────┘
```

| Carte | Rôle |
|---|---|
| STM32F103C8T6 | Acquisition des capteurs, logique de régulation, pilotage des actionneurs |
| ESP32 | Wi-Fi, serveur web, relais des commandes vers le STM32 |

## Matériel

| Élément | Référence utilisée |
|---|---|
| Microcontrôleur temps réel | STM32F103C8T6 (Blue Pill) |
| Passerelle Wi-Fi | ESP32 DevKit |
| Température | LM35 |
| Gaz | MQ-2 (sortie analogique) |
| Présence | PIR HC-SR501 |
| Porte | Servomoteur (SG90 ou équivalent) |
| Lumière / ventilateur | Modules relais ou LED |
| Alarme | Buzzer actif |

## Câblage

| Signal | Broche STM32 |
|---|---|
| LM35 | PA0 |
| MQ-2 (AO) | PA1 |
| PIR | PA4 |
| Servo (signal) | PA6 |
| Lumière | PB0 |
| Ventilateur | PB1 |
| Buzzer | PB5 |
| UART TX / RX | PA9 / PA10 |

| STM32 | ESP32 |
|---|---|
| PA9 (TX) | GPIO16 (RX2) |
| PA10 (RX) | GPIO17 (TX2) |
| GND | GND |

Les deux cartes fonctionnent en 3,3 V : pas de convertisseur de niveau nécessaire. Alimente le servo et les relais avec une source 5 V adaptée, en reliant toutes les masses ensemble.

## Structure du dépôt

```
Smart-House/
├── esp32/smart_house_esp32/
│   ├── smart_house_esp32.ino     Serveur web + passerelle UART
│   └── secrets.h.example         Modèle pour les identifiants Wi-Fi
├── stm32/Core_user_code/
│   ├── app.h                     Broches, seuils, constantes
│   └── app.c                     Capteurs, logique, protocole UART
├── docs/
│   ├── protocole_uart.md         Format des messages et API HTTP
│   └── configuration_cubemx.md   Réglages STM32CubeMX pas à pas
└── README.md
```

## Installation

### STM32
1. Suis `docs/configuration_cubemx.md` pour créer le projet et intégrer `app.h` / `app.c`.
2. Compile et flashe ( Keil MDK-ARM).

### ESP32
1. Copie `secrets.h.example` en `secrets.h` et renseigne ton Wi-Fi.
2. Dans l'Arduino IDE, installe la carte « ESP32 » (gestionnaire de cartes), sélectionne ta carte, puis téléverse le sketch.
3. Ouvre le moniteur série (115200) pour lire l'adresse IP.

### Utilisation
Relie les cartes selon le câblage, puis ouvre `http://<adresse-ip-esp32>` dans un navigateur du même réseau.

## Réglages

Tous les seuils sont dans `app.h` : température de déclenchement du ventilateur, seuil d'alarme gaz, angles du servo, niveau actif des relais (`OUTPUT_ACTIVE_LEVEL`).



## Statut

Code écrit pour un montage standard et vérifié sur le plan syntaxique ; à tester sur le matériel réel.

## Auteur

Islem Bouchoucha — ENSI, Université de la Manouba
