#!/usr/bin/env python3
import argparse
import os
import tempfile
import wave

import paho.mqtt.client as mqtt
import speech_recognition as sr


sessions = {}


def parse_payload(payload: bytes) -> dict:
    text = payload.decode("utf-8", errors="replace").strip()
    values = {}
    for part in text.split(";"):
        if "=" in part:
            key, value = part.split("=", 1)
            values[key] = value
    return values


def write_wav(path: str, pcm: bytes, sample_rate: int, channels: int):
    with wave.open(path, "wb") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(pcm)


def recognize_wav(path: str) -> str:
    recognizer = sr.Recognizer()
    with sr.AudioFile(path) as source:
        audio_data = recognizer.record(source)
    return recognizer.recognize_google(audio_data)


def publish_result(client: mqtt.Client, prefix: str, session: str, text: str):
    client.publish(f"{prefix}/result", f"session={session};text={text}", qos=0)


def on_message(client: mqtt.Client, userdata, message: mqtt.MQTTMessage):
    prefix = userdata["prefix"]
    topic = message.topic
    suffix = topic[len(prefix) + 1:]
    values = parse_payload(message.payload)
    session = values.get("session")
    if not session:
        return

    if suffix == "begin":
        sessions[session] = {
            "rate": int(values.get("rate", "16000")),
            "channels": int(values.get("channels", "1")),
            "chunks": {},
        }
        print(f"begin {session}")

    elif suffix == "chunk":
        state = sessions.get(session)
        if not state:
            return
        index = int(values.get("index", "0"))
        data = values.get("data", "")
        state["chunks"][index] = bytes.fromhex(data)

    elif suffix == "end":
        state = sessions.pop(session, None)
        if not state:
            return

        pcm = b"".join(state["chunks"][i] for i in sorted(state["chunks"]))
        wav_path = None
        try:
            with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as wav_file:
                wav_path = wav_file.name
            write_wav(wav_path, pcm, state["rate"], state["channels"])
            text = recognize_wav(wav_path)
        except sr.UnknownValueError:
            text = ""
        except Exception as exc:
            text = f"ERROR: {exc}"
        finally:
            if wav_path and os.path.exists(wav_path):
                os.remove(wav_path)

        publish_result(client, prefix, session, text)
        print(f"result {session}: {text}")


def main():
    parser = argparse.ArgumentParser(description="MQTT speech-recognition server for pxt-mqtt")
    parser.add_argument("--host", required=True, help="MQTT broker host")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--prefix", default="speech/microbit", help="Speech topic prefix")
    parser.add_argument("--username")
    parser.add_argument("--password")
    args = parser.parse_args()

    client = mqtt.Client(userdata={"prefix": args.prefix})
    if args.username:
        client.username_pw_set(args.username, args.password or "")
    client.on_message = on_message
    client.connect(args.host, args.port, 60)
    client.subscribe(f"{args.prefix}/#", qos=0)
    print(f"listening on {args.host}:{args.port} topic {args.prefix}/#")
    client.loop_forever()


if __name__ == "__main__":
    main()
