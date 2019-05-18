open RespectWrapper.Dsl.Resync
;;

let handler _ res =  res |> Http.Response.end_("Hello, world")
;;

describe "Server" [
  it "starts with a failing test" (fun _ ->
      Supertest.request(handler)
      |> Supertest.get("/")
      |> Supertest.expectStatus(200)
      |> Supertest.endAsync
    )
] |> register;
