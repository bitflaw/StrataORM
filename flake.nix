{
  description = "Strata ORM";
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-25.11";
    mdbcxx.url = "github:bitflaw/mdbcxx";
  };
  outputs =
    {
      self,
      nixpkgs,
      mdbcxx,
    }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };

      mkORM =
        { dbBackend }:
        pkgs.stdenv.mkDerivation {
          pname = "strata-${pkgs.lib.toLower dbBackend}";
          version = "0.2.1";
          src = ./.;

          nativeBuildInputs = [ pkgs.cmake ];

          cmakeFlags = [
            "-DDB_ENGINE=${dbBackend}"
          ];

          buildInputs =
            with pkgs;
            if dbBackend == "PSQL" then
              [
                libpq
                libpqxx
              ]
            else if dbBackend == "MARIADB" then
              [ mdbcxx.packages.${system}.default ]
            else
              [ ];
        };
    in
    {
      packages.${system} = {
        default = mkORM { dbBackend = "PSQL"; };
        postgres = mkORM { dbBackend = "PSQL"; };
        mariadb = mkORM { dbBackend = "MARIADB"; };
      };

      devShells.${system}.default = pkgs.mkShell {
        buildInputs = with pkgs; [
          cmake
          clang-tools

          # for psql
          libpqxx
          libpq

          # for mariadb, draws in mariadb-c
          mdbcxx.packages.${system}.default
        ];
        shellHook = ''
          export SHELL="${pkgs.bashInteractive}/bin/bash"
        '';
      };
    };
}
