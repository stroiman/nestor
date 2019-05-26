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

let test ?expectStatus ?expectBody ~path handler =
    Supertest.request(handler)
    |> Supertest.get(path)
    |> (match expectStatus with
        | Some(e) -> Supertest.expectStatus(e)
        | None -> fun x -> x)
    |> (match expectBody with
        | Some(e) -> Supertest.expectBody(e)
        | None -> fun x -> x)
    |> Supertest.endAsync
;;

describe "Server" [
  it "starts with a failing test" (fun _ ->
      createServer(sendText "Hello, World!") |> test
        ~path: "/"
        ~expectStatus: 200
        ~expectBody: "Hello, World!"
    );

  it "Routes A" (fun _ ->
      server2 |> test
        ~path: "/a"
        ~expectStatus: 200
        ~expectBody: "Got A"
    );

  it "Routes B" (fun _ ->
      server2 |> test
        ~path: "/b"
        ~expectStatus: 200
        ~expectBody: "Got B"
    );

  it "Routes /c/a" (fun _ ->
      server2 |> test
        ~path: "/c/a"
        ~expectStatus: 200
        ~expectBody: "Got CA"
    );

  it "Routes /c/b" (fun _ ->
      server2 |> test
        ~path: "/c/b"
        ~expectStatus: 200
        ~expectBody: "Got CB"
    );

  it "Handles cookies - new syntax - missing cookie" (fun _ ->
      getCookie "Auth"
        ~onMissing:(sendText "Missing Cookie")
      >=>
      (fun (cookie) -> sendText("Hello, " ^ cookie) ())
      |> createServer |> test
        ~path: "/"
        ~expectStatus: 200
        ~expectBody: "Missing Cookie"
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
