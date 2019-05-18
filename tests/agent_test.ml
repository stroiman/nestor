open Respect.Dsl.Sync
open Respect.Matcher
;;

describe "Server" [
  it "starts with a failing test" (fun _ ->
      1 |> should (equal 1)
    )
] |> register;
