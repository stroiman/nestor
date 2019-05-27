open RespectWrapper.Dsl.Resync;
open Index;
open Index.Handler;

let doneSync = (res, cb) => cb(Done(res));

let server =
  router([
    path("/testMethod")
    >=> router([
          get >=> (() => sendText("GET")),
          post >=> (() => sendText("POST")),
        ]),
    path("/testStatus")
    >=> router([
          path("/ok")
          >=> ((_, _req, res) => Response.status(200, res) |> doneSync),
          path("/server-error")
          >=> ((_, _req, res) => Response.status(500, res) |> doneSync),
        ]),
    path("/testCookie")
    >=> getCookie("Auth", ~onMissing=() => sendText("Missing Cookie"))
    >=> (cookie => sendText("Hello, " ++ cookie)),
  ])
  |> createServer;

describe(
  "Server",
  [
    it("Handles cookies", _ =>
      Supertest.request(server)
      |> Supertest.get("/testCookie")
      |> Supertest.set("cookie", "Auth=John")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Hello, John")
      |> Supertest.endAsync
    ),
    it("Handles scan path", _ => {
      let handler =
        scanPath("/users/%s", (userId, _) => sendText("Hello, " ++ userId));

      Supertest.request(createServer(handler))
      |> Supertest.get("/users/john")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Hello, john")
      |> Supertest.endAsync;
    }),
    describe(
      "GET/POST",
      [
        it("Handles GET", _ =>
          Supertest.request(server)
          |> Supertest.get("/testMethod")
          |> Supertest.expectBody("GET")
          |> Supertest.endAsync
        ),
        it("Handles POST", _ =>
          Supertest.request(server)
          |> Supertest.post("/testMethod")
          |> Supertest.expectBody("POST")
          |> Supertest.endAsync
        ),
      ],
    ),
    describe(
      "Status codes",
      [
        it("Handles OK", _ =>
          Supertest.request(server)
          |> Supertest.get("/testStatus/ok")
          |> Supertest.expectStatus(200)
          |> Supertest.endAsync
        ),
        it("Handles POST", _ =>
          Supertest.request(server)
          |> Supertest.post("/testStatus/server-error")
          |> Supertest.expectStatus(500)
          |> Supertest.endAsync
        ),
      ],
    ),
  ],
)
|> register;
