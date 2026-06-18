# Instrukcja dla Claude Code — Robot Stream Viewer (Qt)

## Cel

Napisz aplikację desktopową w **Python + PyQt6** która:
1. Odbiera przez UDP strumień RGBD + audio z robota (Jetson Nano)
2. Wyświetla obraz kolorowy, obraz głębi i poziomy audio w czasie rzeczywistym
3. Pozwala skonfigurować port nasłuchiwania

---

## Stos technologiczny

- Python 3.10+
- PyQt6
- numpy
- opencv-python (tylko do dekodowania JPEG i colormapa — bez okien cv2)
- Brak innych zależności

Zależności do `requirements.txt`:
```
PyQt6
numpy
opencv-python
```

---

## Protokół UDP (zaimplementowany po stronie robota)

### Nagłówek każdego pakietu — 15 bajtów, big-endian

```python
import struct
HEADER_FMT  = '!IBIHHH'   # łącznie 15 bajtów
HEADER_SIZE = struct.calcsize(HEADER_FMT)  # == 15

# Pola:
# I  — magic       uint32  zawsze 0x524F5343 ('ROSC'), odrzuć pakiet jeśli inne
# B  — stream_id   uint8   0=color, 1=depth, 2=audio
# I  — frame_id    uint32  numer klatki (per stream, owijany)
# H  — frag_id     uint16  indeks fragmentu (0-based)
# H  — frag_total  uint16  całkowita liczba fragmentów tej klatki
# H  — payload_len uint16  liczba bajtów danych w tym pakiecie
```

Po nagłówku następuje `payload_len` bajtów danych.

### Fragmentacja

Duże klatki są dzielone na fragmenty ≤ 60 000 bajtów.
Złóż pełną klatkę gdy `len(zebrane_fragmenty) == frag_total`.
Klucz bufora: `(stream_id, frame_id)`.
Złożona klatka = `b''.join(frags[i] for i in range(frag_total))`.

---

## Formaty danych po złożeniu

### stream_id = 0 — Color

- JPEG-encoded BGR image (640×480 typowo)
- Dekoduj: `cv2.imdecode(np.frombuffer(data, np.uint8), cv2.IMREAD_COLOR)`
- Wynik: ndarray (H, W, 3) BGR → konwertuj do RGB przed wyświetleniem w Qt

### stream_id = 1 — Depth

- Pierwsze 4 bajty: `struct.unpack('!HH', data[:4])` → `(height, width)`
- Pozostałe bajty: zlib-compressed raw uint16
- Dekoduj:
  ```python
  import zlib, struct
  h, w = struct.unpack('!HH', data[:4])
  raw  = zlib.decompress(data[4:])
  depth = np.frombuffer(raw, dtype=np.uint16).reshape(h, w)
  ```
- Wartości w milimetrach (0 = brak pomiaru)
- Do wyświetlenia: znormalizuj do 0–255, zastosuj colormap TURBO z OpenCV:
  ```python
  valid = depth[depth > 0]
  if valid.size:
      norm = np.clip((depth.astype(np.float32) - valid.min()) /
                     (valid.max() - valid.min() + 1e-6), 0, 1)
  else:
      norm = np.zeros_like(depth, dtype=np.float32)
  grey     = (norm * 255).astype(np.uint8)
  colormap = cv2.applyColorMap(grey, cv2.COLORMAP_TURBO)  # BGR
  # konwertuj BGR→RGB przed Qt
  ```

### stream_id = 2 — Audio

- Raw PCM signed 16-bit little-endian, interleaved
- 6 kanałów (ReSpeaker 4-Mic Array: 4 surowe miki + 2 przetworzone)
- 16 000 Hz sample rate
- ~512 ramek na pakiet (≈ 32 ms)
- Dekoduj: `np.frombuffer(data, dtype=np.int16).reshape(-1, 6)`
- Wynik: ndarray (frames, 6)

---

## Wymagania UI

### Układ okna głównego

```
┌─────────────────────────────────────────────────────┐
│  [● połączono / ○ rozłączono]   Port: [5005] [Start]│
├───────────────────────┬─────────────────────────────┤
│                       │                             │
│     Color image       │       Depth image           │
│     (640×480)         │    (colormap TURBO)         │
│                       │                             │
├───────────────────────┴─────────────────────────────┤
│  Audio — 6 słupkowych wskaźników poziomu (VU meter) │
│  Ch1 ████░░░░  Ch2 ██░░░░░░  Ch3 ███░░░░  ...       │
├─────────────────────────────────────────────────────┤
│  Color: 28 fps   Depth: 28 fps   Audio: 31 fps      │
└─────────────────────────────────────────────────────┘
```

### Szczegóły komponentów

**Obrazy (QLabel z QPixmap):**
- Skaluj proporcjonalnie do dostępnej przestrzeni (`Qt.AspectRatioMode.KeepAspectRatio`)
- Minimum 320×240 każdy

**VU meter audio:**
- 6 pionowych lub poziomych pasków `QProgressBar` (jeden per kanał)
- Wartość = RMS ostatniego pakietu dla danego kanału
- Skaluj: `rms / 32768 * 100` → wartość 0–100
- Etykiety: Ch1, Ch2, Ch3, Ch4, Ch5, Ch6

**Pasek statusu (QStatusBar):**
- FPS osobno dla każdego strumienia (uśredniaj po ostatnich 30 klatkach)
- Format: `Color: XX fps  |  Depth: XX fps  |  Audio: XX fps`

**Kontrolki połączenia:**
- `QSpinBox` dla portu (zakres 1024–65535, domyślnie 5005)
- Przycisk Start/Stop
- Wskaźnik LED (zielony = dane napływają, szary = brak)

---

## Architektura — wątki

**Wątek główny (Qt):** tylko UI

**ReceiverThread (QThread):**
- Nasłuchuje UDP socket (`SO_REUSEADDR`)
- Timeout 0.5s na `recvfrom` aby móc sprawdzić flagę stop
- Po złożeniu kompletnej klatki emituje sygnał:
  ```python
  color_ready  = pyqtSignal(np.ndarray)   # (H,W,3) RGB uint8
  depth_ready  = pyqtSignal(np.ndarray)   # (H,W) uint16
  audio_ready  = pyqtSignal(np.ndarray)   # (frames,6) int16
  ```
- Sloty w MainWindow odbierają sygnały i aktualizują widżety

**Konwersja ndarray → QPixmap (helper):**
```python
from PyQt6.QtGui import QImage, QPixmap
def ndarray_to_pixmap(rgb: np.ndarray) -> QPixmap:
    h, w, ch = rgb.shape
    img = QImage(rgb.data, w, h, w * ch, QImage.Format.Format_RGB888)
    return QPixmap.fromImage(img)
```

---

## Struktura plików do stworzenia

```
robot_viewer/
├── requirements.txt
├── main.py              ← entry point, tworzy QApplication i MainWindow
├── receiver.py          ← ReceiverThread: UDP odbiór + reassembly + dekodowanie
├── main_window.py       ← MainWindow: układ UI, sloty
└── widgets.py           ← VuMeter (QWidget z QProgressBar-ami), ImageLabel
```

---

## Szczegóły implementacji

### receiver.py — kluczowe fragmenty

```python
from PyQt6.QtCore import QThread, pyqtSignal
import struct, socket, zlib
import numpy as np
import cv2
from collections import defaultdict

MAGIC = 0x524F5343
HEADER_FMT  = '!IBIHHH'
HEADER_SIZE = struct.calcsize(HEADER_FMT)

class ReceiverThread(QThread):
    color_ready = pyqtSignal(object)   # np.ndarray RGB
    depth_ready = pyqtSignal(object)   # np.ndarray uint16
    audio_ready = pyqtSignal(object)   # np.ndarray int16

    def __init__(self, port: int):
        super().__init__()
        self._port   = port
        self._stop   = False
        self._frags  = defaultdict(dict)

    def stop(self):
        self._stop = True

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(('0.0.0.0', self._port))
        sock.settimeout(0.5)

        while not self._stop:
            try:
                raw, _ = sock.recvfrom(65536)
            except socket.timeout:
                continue

            if len(raw) < HEADER_SIZE:
                continue

            magic, sid, fid, frag_id, frag_total, plen = \
                struct.unpack(HEADER_FMT, raw[:HEADER_SIZE])

            if magic != MAGIC:
                continue

            payload = raw[HEADER_SIZE:HEADER_SIZE + plen]
            key     = (sid, fid)
            self._frags[key][frag_id] = payload

            if len(self._frags[key]) == frag_total:
                full = b''.join(self._frags[key][i] for i in range(frag_total))
                del self._frags[key]
                self._dispatch(sid, full)

        sock.close()

    def _dispatch(self, sid: int, data: bytes):
        if sid == 0:    # color
            arr   = np.frombuffer(data, dtype=np.uint8)
            frame = cv2.imdecode(arr, cv2.IMREAD_COLOR)
            if frame is not None:
                self.color_ready.emit(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))

        elif sid == 1:  # depth
            h, w  = struct.unpack('!HH', data[:4])
            raw   = zlib.decompress(data[4:])
            depth = np.frombuffer(raw, dtype=np.uint16).reshape(h, w)
            self.depth_ready.emit(depth)

        elif sid == 2:  # audio
            samples = np.frombuffer(data, dtype=np.int16).reshape(-1, 6)
            self.audio_ready.emit(samples)
```

### main_window.py — szkielet slotów

```python
def _on_color(self, frame: np.ndarray):
    self._fps_color.tick()
    self.color_label.setPixmap(
        ndarray_to_pixmap(frame).scaled(
            self.color_label.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation))

def _on_depth(self, depth: np.ndarray):
    self._fps_depth.tick()
    valid = depth[depth > 0]
    if valid.size:
        norm = np.clip((depth.astype(np.float32) - valid.min()) /
                       (valid.max() - valid.min() + 1e-6), 0, 1)
    else:
        norm = np.zeros_like(depth, dtype=np.float32)
    grey = (norm * 255).astype(np.uint8)
    bgr  = cv2.applyColorMap(grey, cv2.COLORMAP_TURBO)
    rgb  = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    self.depth_label.setPixmap(
        ndarray_to_pixmap(rgb).scaled(
            self.depth_label.size(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation))

def _on_audio(self, samples: np.ndarray):
    self._fps_audio.tick()
    rms = np.sqrt(np.mean(samples.astype(np.float32) ** 2, axis=0))
    levels = (rms / 32768.0 * 100).astype(int).clip(0, 100)
    self.vu_meter.set_levels(levels)
```

### FPS counter (helper)

```python
import time
from collections import deque

class FpsCounter:
    def __init__(self, window=30):
        self._times = deque(maxlen=window)

    def tick(self):
        self._times.append(time.monotonic())

    @property
    def fps(self) -> float:
        if len(self._times) < 2:
            return 0.0
        return (len(self._times) - 1) / (self._times[-1] - self._times[0])
```

---

## Zachowanie aplikacji

- Uruchom: `python3 main.py`
- Okno od razu wyświetla się (bez danych — szare panele)
- Kliknięcie **Start** otwiera socket i zaczyna nasłuchiwać
- Kliknięcie **Stop** zamyka socket i zatrzymuje wątek
- Zamknięcie okna automatycznie zatrzymuje wątek odbiorczy
- Pasek statusu aktualizuje FPS co 1 sekundę (`QTimer`)
- Wskaźnik LED zmienia kolor na zielony gdy czas od ostatniego pakietu < 2s

---

## Czego NIE robić

- Nie używaj `cv2.imshow` ani `cv2.waitKey` — tylko Qt
- Nie blokuj głównego wątku Qt żadnymi operacjami sieciowymi
- Nie twórz socket w głównym wątku
- Nie używaj `time.sleep` w wątku Qt

---

## Koniec — uruchomienie

```bash
cd robot_viewer
pip install -r requirements.txt
python3 main.py
```

Robot musi mieć uruchomiony `comms_node` z parametrem `host` ustawionym na IP tego laptopa.
