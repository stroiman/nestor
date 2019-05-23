open RespectWrapper.Dsl.Resync;
open Index;
open Index.Handler;

describe(
  "Server",
  [
    it("Handles cookies - new syntax", _ => {
      let handler =
        getCookie("Auth", ~onMissing=sendText("Missing Cookie"))
        >=> (cookie => sendText("Hello, " ++ cookie, ()));

      Supertest.request(createServer(handler))
      |> Supertest.get("/b")
      |> Supertest.set("cookie", "Auth=John")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Hello, John")
      |> Supertest.endAsync;
    }),
  ],
)
|> register;
