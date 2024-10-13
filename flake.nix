{
  description = "An OpenGL Hello World";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    nixgl.url   = "github:nix-community/nixGL";
  };

  outputs = { self, nixpkgs, nixgl }:
    let
      build_for = system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [ nixgl.overlay ];
          };
        in

        pkgs.stdenv.mkDerivation {
          name = "helloWorld";
          src = self;
          buildInputs = [ 
            pkgs.gcc
            
            pkgs.nixgl.auto.nixGLDefault

            pkgs.libGL
            pkgs.glfw

            pkgs.stb
          ];
          buildPhase  = "gcc -o helloWorld src/* -lm -lglfw -lGL -Iinclude";
          installPhase = "mkdir -p $out/bin; install -t $out/bin helloWorld";
        };
      
    in
    {
      packages.x86_64-linux.default = build_for "x86_64-linux";
    };
}
