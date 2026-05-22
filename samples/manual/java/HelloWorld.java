// samples/manual/java/HelloWorld.java — Minimal OCCT-Light Java sample.
//
// Build and run (from repo root, after building the Java binding):
//   java --class-path bindings/java/build/classes/java/main:bindings/java/build/deps/*
//        samples/manual/java/HelloWorld.java

import org.occtl.*;
import java.util.List;

public class HelloWorld {
    public static void main(String[] args) {
        Graph g = Graph.create();

        NodeId boxA = g.makeBox(10.0, 10.5, 10.0);
        System.out.println("Box A: " + boxA);

        NodeId boxB = g.makeBox(10.0, 10.0, 10.0);
        System.out.println("Box B: " + boxB);

        NodeId fused = g.fuse(List.of(boxA), List.of(boxB));
        System.out.println("Fused root: " + fused);

        System.out.println("Done.");
    }
}
