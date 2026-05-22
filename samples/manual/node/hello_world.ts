// samples/manual/node/hello_world.ts — Minimal OCCT-Light Node.js sample.
//
// Run:
//   npx tsx samples/manual/node/hello_world.ts

import { Graph, NodeId } from "occtl";

const g = Graph.create();

const boxA: NodeId = g.makeBox({ dx: 10.0, dy: 10.5, dz: 10.0 });
console.log("Box A:", boxA);

const boxB: NodeId = g.makeBox({ dx: 10.0, dy: 10.0, dz: 10.0 });
console.log("Box B:", boxB);

const fused: NodeId = g.fuse([boxA], [boxB]);
console.log("Fused root:", fused);

console.log("Writing fused.brep...");
g.writeBrep(fused, "fused.brep");
console.log("Done.");
