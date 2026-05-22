# Samples

Two categories of samples live here:

- `automatic/` — script-generated model artifacts (Python registry + runner)
- `manual/` — hand-written mini-samples in each supported language, demonstrating the same workflow (build, fuse, write BREP)

## Manual samples

Each language sample does the same thing:
1. Initialise the runtime
2. Create a graph
3. Build two boxes
4. Fuse them
5. Write the result to `fused.brep`

**C++** — uses the header-only C++ veneer  
**Python** — uses the `occtl` package  
**C#** — uses the `OcctL` namespace  
**Node** — uses the `occtl` npm package  
**Java** — uses the `org.occtl` package
