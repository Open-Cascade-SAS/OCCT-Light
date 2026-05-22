# OCCT-Light Automatic Samples (Python)

This folder contains registered Python sample builders and a runner for
generating model artifacts.

Run from a build where the Python binding can load the native library:

```bash
python3 samples/automatic/run_samples.py --output build/samples/python
```

The runner builds each registered topology sample, validates it with
`occtl_graph_check`, writes a `.brep` CAD model, and renders an offscreen PNG
from OCCT RGBA pixels. Use `--reports-only` on machines without an OCCT
visualization driver.

The registry intentionally includes both small primitive smoke cases and
larger CAD-style models. The complex family covers dense boolean-drilled
plates, selected-edge chamfers and fillets, offset plates, thick-solid hollow
trays, ribbed brackets, and finned heat sinks.
