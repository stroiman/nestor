open RespectWrapper.Dsl.Resync
open Index
open Handler
open Middlewares

[@@@ocaml.warning "-44"]
let  getBodyString (req: Index.Request.t) (_res: Index.Response.t) =
  let open Async in
  Request.Body.bodyStringA req.body
  >>= (fun s -> Handler.continue s)
[@@@ocaml.warning "+44"]

(* Request.Body.bodyString req.body (fun s -> cb(Continue (s, req))) *)

let asJson (bodyString: string) (req: Request.t) (res: Response.t) =
  match (Request.getHeader("content-type")(req)) with
  | Some("application/json") ->
    (try
       let json = Js.Json.parseExn(bodyString) in
       continue json
     with _ ->
       done_ (res |> Response.status(400) |> Response.send("Invalid JSON")))
  | _ ->
    done_ (res |> Response.status(400) |> Response.send("Missing content type"))

let getBodyJson: Js.Json.t Handler.t = getBodyString >>= asJson

type user = {
  firstName: string;
  lastName: string;
}

let decodeUser js =
  let open JsonDecoder in
  let make firstName lastName = { firstName; lastName } in
  decodeObject (
    make
    <$> (decodeField "firstName" decodeString)
    <*> (decodeField "lastName" decodeString)
  ) |> run js

let decode decoder a _req res =
  match decoder a with
  | Some(x) -> continue x
  | None -> done_(res |> Response.status(400) |> Response.send("Cannot decode"))

let handler: (unit) Handler.t =
  getBodyJson >>= (decode decodeUser) >>=
  (fun user -> sendText ("Hello, " ^ user.firstName ^ " " ^ user.lastName))

let server = createServer @@ handler

;;
describe "Json Deserializing" [
  it "Returns 400 when not sending content-type" (fun _ ->
      Supertest.request(server)
      |> Supertest.get("/b")
      |> Supertest.expectStatus(400)
      |> Supertest.expectBody("Missing content type")
      |> Supertest.endAsync
    );

  it "Returns 400 when not sending valid json" (fun _ ->
      Supertest.request(server)
      |> Supertest.post("/b")
      |> Supertest.set "content-type" "application/json"
      |> Supertest.sendString("This is not valid json")
      |> Supertest.expectStatus(400)
      |> Supertest.expectBody("Invalid JSON")
      |> Supertest.endAsync
    );

  it "Returns 400 when not sending wrong json" (fun _ ->
      Supertest.request(server)
      |> Supertest.post("/b")
      |> Supertest.set "content-type" "application/json"
      |> Supertest.sendString("{ \"firstName\": \"John\" }")
      |> Supertest.expectStatus(400)
      |> Supertest.expectBody("Cannot decode")
      |> Supertest.endAsync
    );

  it "Returns 200 when not sending right json" (fun _ ->
      Supertest.request(server)
      |> Supertest.post("/b")
      |> Supertest.set "content-type" "application/json"
      |> Supertest.sendString("{ \"firstName\": \"John\", \"lastName\": \"Doe\" }")
      |> Supertest.expectStatus(200)
      |> Supertest.expectBody("Hello, John Doe")
      |> Supertest.endAsync
    );
] |> register
