#!/usr/bin/env python3
import argparse
import io
import json
import re
import wave

import requests
import serial


BEGIN_RE = re.compile(rb"#I2S_BEGIN\s+rate=(\d+)\s+bits=(\d+)\s+channels=(\d+)")
CHUNK_RE = re.compile(rb"#I2S_CHUNK\s+(\d+)")


def pcm_to_wav(pcm: bytes, sample_rate: int, channels: int) -> bytes:
    out = io.BytesIO()
    with wave.open(out, "wb") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(pcm)
    return out.getvalue()


def extract_text(response: requests.Response) -> str:
    content_type = response.headers.get("content-type", "")
    if "application/json" in content_type:
        data = response.json()
        if isinstance(data, dict):
            for key in ("text", "transcript", "result"):
                value = data.get(key)
                if isinstance(value, str):
                    return value
        return json.dumps(data, ensure_ascii=False)
    return response.text.strip()


def post_to_backend(url: str, wav_bytes: bytes) -> str:
    files = {"file": ("microbit-i2s.wav", wav_bytes, "audio/wav")}
    response = requests.post(url, files=files, timeout=60)
    response.raise_for_status()
    return extract_text(response)


def read_capture(port: serial.Serial):
    line = port.readline()
    match = BEGIN_RE.search(line)
    if not match:
        return None

    sample_rate = int(match.group(1))
    bits = int(match.group(2))
    channels = int(match.group(3))
    if bits != 16:
        raise ValueError(f"Only 16-bit PCM is supported, got {bits}")

    payload = bytearray()
    while True:
        line = port.readline().strip()
        if line == b"#I2S_END":
            return sample_rate, channels, bytes(payload)

        chunk = CHUNK_RE.search(line)
        if chunk:
            size = int(chunk.group(1))
            payload.extend(port.read(size))


def main():
    parser = argparse.ArgumentParser(description="micro:bit I2S microphone speech-recognition gateway")
    parser.add_argument("--port", required=True, help="Serial port, for example /dev/tty.usbmodem1102 or COM5")
    parser.add_argument("--backend", required=True, help="Speech backend URL that accepts multipart field 'file'")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=1) as port:
        print(f"Listening on {args.port}")
        while True:
            capture = read_capture(port)
            if capture is None:
                continue

            sample_rate, channels, pcm = capture
            wav_bytes = pcm_to_wav(pcm, sample_rate, channels)
            try:
                text = post_to_backend(args.backend, wav_bytes)
            except Exception as exc:
                text = f"ERROR: {exc}"

            port.write(("#I2S_TEXT " + text.replace("\n", " ") + "\n").encode("utf-8"))
            port.flush()
            print(text)


if __name__ == "__main__":
    main()
