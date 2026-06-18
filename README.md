# Ego — Camera + Microphone → Compute pipeline

## Architektura

```
[camera_node C++]      ──/camera/color──▶
                        ──/camera/depth──▶  [compute_node Python]
[microphone_node C++]  ──/audio──────────▶
```

| Węzeł | Język | Co robi |
|---|---|---|
| `camera_node` | C++ | Otwiera RealSense przez librealsense2, publikuje obraz RGB (`sensor_msgs/Image`) i głębię (`sensor_msgs/Image`) |
| `microphone_node` | C++ | Otwiera ReSpeaker 4-mic przez ALSA, publikuje 512-klatkowe bufory PCM (`std_msgs/Int16MultiArray`, 6 kanałów, 16 kHz) |
| `compute_node` | Python | Subskrybuje oba topiki, przechowuje najnowszą klatkę każdego w zmiennych instancji — **aktualnie szkielet; właściwa logika do dopisania** |

---

## 1. Instalacja ROS2 Eloquent

> Jetson Nano z JetPack 4.x działa na Ubuntu 18.04 (Bionic) + Python 3.6.
> Wspierana wersja ROS2 to **Eloquent Elusor**.

```bash
# Ustawienie locale
sudo locale-gen en_US en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
export LANG=en_US.UTF-8

# Dodanie klucza i repozytorium ROS2
sudo apt install -y curl gnupg2 lsb-release
curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo apt-key add -
sudo sh -c 'echo "deb [arch=$(dpkg --print-architecture)] http://packages.ros.org/ros2/ubuntu \
     $(lsb_release -cs) main" > /etc/apt/sources.list.d/ros2-latest.list'

sudo apt update
# Minimal install (bez GUI) — wystarczy na Jetsonie
sudo apt install -y ros-eloquent-ros-base python3-colcon-common-extensions
```

---

## 2. Instalacja librealsense2

Intel nie dostarcza gotowych paczek dla Jetson arm64 — trzeba zbudować ze źródeł.

```bash
sudo apt install -y git cmake libssl-dev libusb-1.0-0-dev pkg-config \
     libgtk-3-dev libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev

git clone https://github.com/IntelRealSense/librealsense.git ~/librealsense
cd ~/librealsense

# Reguły udev dla RealSense
sudo cp config/99-realsense-libusb.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger

mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_GRAPHICAL_EXAMPLES=OFF \
  -DFORCE_LIBUVC=ON          # wymagane na Jetsonie (brak kernel patch)
make -j$(nproc)
sudo make install
sudo ldconfig
```

Weryfikacja (podłącz RealSense):
```bash
realsense-viewer        # GUI
# lub
rs-enumerate-devices    # lista kamer
```

---

## 3. Instalacja zależności ROS

```bash
sudo apt install -y \
  ros-eloquent-cv-bridge \
  ros-eloquent-sensor-msgs \
  ros-eloquent-std-msgs \
  libopencv-dev \
  libasound2-dev   # ALSA — już zainstalowane
```

---

## 4. Sprawdzenie nazwy urządzenia ReSpeaker

ReSpeaker 4-Mic Array podłącza się przez USB i pojawia jako urządzenie ALSA.

```bash
# Wypisz wszystkie urządzenia nagrywające
arecord -l
```

Przykładowy wynik:
```
card 1: ReSpeaker4MicArr [ReSpeaker 4 Mic Array (UAC1.0)], device 0: ...
```

Nazwa do użycia w parametrze `device` węzła: `plughw:CARD=ReSpeaker4MicArr,DEV=0`  
(lub `plughw:1,0` jeśli karta ma numer 1).

Szybki test nagrywania:
```bash
arecord -D plughw:CARD=ReSpeaker4MicArr,DEV=0 -f S16_LE -r 16000 -c 6 test.wav
```

Jeśli inna nazwa karty, zaktualizuj parametr `device` w `launch/system.launch.py`.

---

## 5. Konfiguracja aliasu ALSA `respeaker`

Launch file przekazuje węzłowi `device: 'respeaker'`. ALSA musi znać ten alias —
zdefiniuj go w `~/.asoundrc` (utwórz plik jeśli nie istnieje):

```
pcm.respeaker {
    type plug
    slave {
        pcm "hw:ReSpeaker4MicArr,0"
    }
}
```

Jeśli karta ma inny numer (sprawdź przez `arecord -l`), zmień `ReSpeaker4MicArr` na
właściwą nazwę lub użyj numeru karty, np. `hw:1,0`.

Weryfikacja aliasu:
```bash
arecord -D respeaker -f S16_LE -r 16000 -c 6 -d 3 test.wav
```

> **PulseAudio:** jeśli system używa PA i pojawia się konflikt przy `snd_pcm_open`,
> zmień typ na `pulse` i podaj nazwę źródła PA zamiast `hw:…`:
> ```
> pcm.respeaker {
>     type pulse
>     device "alsa_input.usb-SEEED_ReSpeaker_4_Mic_Array__UAC1.0_-00.multichannel-input"
> }
> ```
> Nazwę źródła PA sprawdzisz przez `pactl list sources short`.

---

## 6. Konfiguracja sieci

Przed uruchomieniem skonfiguruj adres IP laptopa:

```bash
cp config/network.example.json config/network.json
# Edytuj config/network.json i wpisz IP swojego laptopa w polu "laptop_ip"
```

Plik `config/network.json` jest w `.gitignore` i nie trafi do repozytorium.

---

## 7. Budowanie pakietu

```bash
# Wejdź do katalogu workspace
cd ~/Ego          # lub inna lokalizacja gdzie sklonowałeś repo

# Źródłuj ROS2 (dostosuj dystrybucję jeśli inna niż eloquent)
source /opt/ros/${ROS_DISTRO:-eloquent}/setup.bash

# Buduj
colcon build --packages-select robot_pipeline --cmake-args -DCMAKE_BUILD_TYPE=Release

# Źródłuj workspace
source install/setup.bash
```

Opcjonalnie — dodaj do `~/.bashrc`, żeby nie wpisywać w każdym terminalu:
```bash
echo "source /opt/ros/eloquent/setup.bash" >> ~/.bashrc
echo "source ~/Ego/install/setup.bash" >> ~/.bashrc
```

---

## 8. Uruchomienie

### Wszystkie węzły naraz (zalecane)

```bash
source /opt/ros/${ROS_DISTRO:-eloquent}/setup.bash
source install/setup.bash

ros2 launch robot_pipeline system.launch.py
```

Lub skrypt który robi wszystko automatycznie:
```bash
./start_pipeline.sh
```

### Węzły osobno (np. do debugowania)

Terminal 1 — kamera:
```bash
ros2 run robot_pipeline camera_node
```

Terminal 2 — mikrofon (opcjonalna zmiana urządzenia):
```bash
ros2 run robot_pipeline microphone_node \
  --ros-args -p device:=plughw:CARD=ReSpeaker4MicArr,DEV=0
```

Terminal 3 — compute:
```bash
ros2 run robot_pipeline compute_node.py
```

---

## 7. Weryfikacja topików

```bash
# Lista aktywnych topików
ros2 topic list

# Częstotliwość
ros2 topic hz /camera/color
ros2 topic hz /audio

# Podgląd nagłówka klatki
ros2 topic echo /camera/color --no-arr
ros2 topic echo /audio --no-arr
```

---

## Struktura plików

```
Ego/
├── config/
│   ├── network.example.json  ← szablon (w repo)
│   └── network.json          ← twój config z IP (w .gitignore!)
├── start_pipeline.sh
└── src/
    └── robot_pipeline/
        ├── package.xml
        ├── CMakeLists.txt
        ├── src/
        │   ├── camera_node.cpp       # C++ — RealSense RGBD
        │   └── microphone_node.cpp   # C++ — ALSA / ReSpeaker
        ├── robot_pipeline/
        │   ├── __init__.py
        │   └── compute_node.py       # Python — trzyma aktualne klatki
        └── launch/
            └── system.launch.py      # startuje wszystkie 3 węzły
```

## Topiki

| Topic | Typ | Producent | Konsument |
|---|---|---|---|
| `/camera/color` | `sensor_msgs/Image` (bgr8) | camera_node | compute_node |
| `/camera/depth` | `sensor_msgs/Image` (mono16, wartości w mm) | camera_node | compute_node |
| `/audio` | `std_msgs/Int16MultiArray` (6ch × 512 próbek) | microphone_node | compute_node |
