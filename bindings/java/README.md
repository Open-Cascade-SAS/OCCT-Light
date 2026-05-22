# OCCT-Light Java Binding (JNA)

Java binding that follows the OCCT-Light binding contract:

- generated raw layer from `build/abi.json`
- thin idiomatic facade for Graph/Prim/Bool flows
- Tier-1 smoke, Tier-2 parity runner, Tier-3 symbol coverage tests

## Regenerate from ABI

From repo root:

```bash
python3 bindings/java/tools/generate_facade.py --abi build/abi.json
```

## Run tests

Set native library location first (example for `build/full-shared`):

```bash
export OCCTL_LIBRARY_PATH="$PWD/build/full-shared/lib"
export OCCTL_LIBRARY_NAME="full"   # also accepts "occtl-full"
```

Then run:

```bash
cd bindings/java
mvn test
```

## Parity runner

```bash
cd bindings/java
mvn -q exec:java \
  -Dexec.mainClass=org.occtl.parity.Runner \
  -Dexec.classpathScope=test \
  -Dexec.args="$PWD/../../tests/binding_parity/build_box.json"
```
