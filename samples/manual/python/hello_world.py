# samples/manual/python/hello_world.py — Minimal OCCT-Light Python sample.
#
# Run from a build where the Python binding can load the native library:
#   python3 samples/manual/python/hello_world.py

import occtl

g = occtl.Graph()

box_a = occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=10.0, dy=10.5, dz=10.0))
print("Box A:", box_a)

box_b = occtl.prim.make_box(g, occtl.prim.BoxInfo(dx=10.0, dy=10.0, dz=10.0))
print("Box B:", box_b)

fused = occtl.bool_.fuse(g, [box_a], [box_b])
print("Fused root:", fused)

print("Writing fused.brep...")
occtl.io_brep.write(g, fused, "fused.brep")
print("Done.")
