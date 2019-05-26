open RespectWrapper.Dsl.Resync;
open Index;

open Index.Handler;

let scanPath = (pattern, f, data, req) =>
  Scanf.sscanf(req |> Request.getPath, pattern, f, data, req);

describe(
  "Server",
  [
    it("Handles cookies", _ => {
      let handler =
        getCookie("Auth", ~onMissing=() => sendText("Missing Cookie"))
        >=> (cookie => sendText("Hello, " ++ cookie));

      Supertest.request(createServer(handler))
      |> Supertest.get("/b")
      |> Supertest.set("cookie", "Auth=John")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Hello, John")
      |> Supertest.endAsync;
    }),
    it("Handles scan path", _ => {
      let handler =
        scanPath("/users/%s", (userId, _) => sendText("Hello, " ++ userId));

      Supertest.request(createServer(handler))
      |> Supertest.get("/users/john")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Hello, john")
      |> Supertest.endAsync;
    }),
  ],
)
|> register;
