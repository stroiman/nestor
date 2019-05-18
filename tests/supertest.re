module Response = {
  type t;

  [@bs.get] external getBody: t => Js.t('a) = "body";
  [@bs.get] external getBodyJson: t => Js.Json.t = "body";
};

module Error = {
  type t = Js.Exn.t;
};
type t;
type errorCallback = (Js.null(Error.t), Http.Response.t) => unit;
[@bs.module] external request: Http.requestListener => t = "supertest";
[@bs.module "supertest"] external agent: Http.requestListener => t = "";
[@bs.send.pipe: t] external get: string => t = "get";
[@bs.send.pipe: t] external post: string => t = "post";
[@bs.send.pipe: t] external send: Js.t('a) => t = "send";
[@bs.send.pipe: t] external sendString: string => t = "send";
[@bs.send.pipe: t] external expectStatus: int => t = "expect";
[@bs.send] external end_: (t, errorCallback) => unit = "end";

let endAsync = x => {
  end_(x) |> Async.from_js;
};
