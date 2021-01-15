# ALSA_WAV_RECORDING

```
arecord -f S16_LE -d 2 -r 44100 --device="hw:2,0" test-mic.wav
```