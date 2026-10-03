# samples/

This directory is intentionally empty in version control.

Drop captured `*.mobibdump` files here while developing decoders. They
contain card-holder-specific data (contracts, journeys, sometimes a
name) and **must not be committed**. The `.gitignore` in this folder
ignores everything except itself and this README.

Pull a dump from a Flipper over CLI:

```sh
python3 - <<'PY'
import serial, time
s = serial.Serial('/dev/ttyACM0', 230400, timeout=1)
def send(cmd, wait=6):
    s.write(cmd.encode()+b'\r')
    out=b''; t=time.time()
    while time.time()-t < wait:
        d=s.read(8192)
        if d: out+=d; t=time.time()
    return out.decode('utf-8','replace')
print(send('storage list /ext/apps_data/mobipper/dumps'))
PY
```
