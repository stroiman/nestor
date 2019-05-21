open RespectWrapper.Dsl.Resync
open Index
open Index.Handler

let router = choose [

    path "a" >=> sendText2 "Got A";
    path "b" >=> sendText "Got B";
  ]

let server2 = createServerM(router)
;;

describe "Server" [
  it "starts with a failing test" (fun _ ->
      let h = createServerM(sendText "Hello, World!") in
      Supertest.request(h)
      |> Supertest.get("/")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Hello, World!")
      |> Supertest.endAsync
    );

  it "Routes A" (fun _ ->
      Supertest.request(server2)
      |> Supertest.get("/a")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Got A")
      |> Supertest.endAsync
    );

  it "Routes B" (fun _ ->
      Supertest.request(server2)
      |> Supertest.get("/b")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Got B")
      |> Supertest.endAsync
    )
] |> register;
