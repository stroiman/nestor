open RespectWrapper.Dsl.Resync
open Index
open Index.Handler

let router = router [
    path "/a" >=> sendText "Got A";
    path "/b" >=> sendText "Got B";
    path "/c/a" >=> sendText "Got CA";
    path "/c" >=> path "/b" >=> sendText "Got CB";
  ]

let server2 = createServer(router)
;;

describe "Server" [
  it "starts with a failing test" (fun _ ->
      let h = createServer(sendText "Hello, World!") in
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
    );

  it "Routes /c/a" (fun _ ->
      Supertest.request(server2)
      |> Supertest.get("/c/a")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Got CA")
      |> Supertest.endAsync
    );

  it "Routes /c/b" (fun _ ->
      Supertest.request(server2)
      |> Supertest.get("/c/b")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Got CB")
      |> Supertest.endAsync
    );

  it "Handles cookies - new syntax - missing cookie" (fun _ ->
      let handler =
        getCookie "Auth"
          ~onMissing:(sendText "Missing Cookie")
        >=>
        (fun (cookie) -> sendText("Hello, " ^ cookie) ())
      in
      Supertest.request(createServer(handler))
      |> Supertest.get("/b")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Missing Cookie")
      |> Supertest.endAsync
    );

  it "Handles cookies - new syntax" (fun _ ->
      let handler =
        getCookie "Auth"
          ~onMissing:(sendText "Missing Cookie")
        >=>
        (fun (cookie) -> sendText("Hello, " ^ cookie) ())
      in
      Supertest.request(createServer(handler))
      |> Supertest.get("/b")
      |> Supertest.set "cookie" "Auth=John"
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Hello, John")
      |> Supertest.endAsync
    );
] |> register;
