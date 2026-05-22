// samples/manual/csharp/HelloWorld.cs — Minimal OCCT-Light C# sample.
//
// Build and run:
//   dotnet run --project samples/manual/csharp/HelloWorld.csproj

using OcctL;

var graph = new Graph();

var boxA = Prim.MakeBox(graph, 10.0, 10.5, 10.0);
Console.WriteLine($"Box A: {boxA}");

var boxB = Prim.MakeBox(graph, 10.0, 10.0, 10.0);
Console.WriteLine($"Box B: {boxB}");

var fused = Bool.Fuse(graph, [boxA], [boxB]);
Console.WriteLine($"Fused root: {fused}");

Console.WriteLine("Writing fused.brep...");
IoBrep.Write(graph, fused, "fused.brep");
Console.WriteLine("Done.");
