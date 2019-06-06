module Request = {
  module Body = {
    type t = NodeModules.Http.Request.Body.t;

    /* [@bs.val] [@bs.scope "JSON"] external toJson: t => Js.Json.t = "parse"; */

    let bodyString = (input: t, cb: string => unit): unit => {
      let on = NodeModules.Http.Request.Body.on;
      let data = ref("");
      input
      ->on("data", d => {
          data := data^ ++ d;
          ();
        });
      input->on("end", _ => cb(data^));
    };
  };

  type t = {
    body: Body.t,
    method: string,
    headers: Js.Dict.t(string),
    cookies: Js.Dict.t(string),
    path: string,
  };

  let getCookie = (name, req) => req.cookies->Js.Dict.get(name);

  let getPath = req => req.path;

  let getHeader = (name, req) => req.headers->Js.Dict.get(name);

  let from_native_request = req => {
    open NodeModules;
    let url = Http.Request.url(req) |> Url.parse;
    let headers = Http.Request.headers(req);
    let cookies =
      headers->Js.Dict.get("cookie")
      |> (
        fun
        | Some(x) => Cookie.parse(x)
        | None => Js.Dict.empty()
      );
    let path = Url.path(url);

    {
      body: Http.Request.body(req),
      path,
      cookies,
      headers,
      method: Http.Request.method(req),
    };
  };
};

module Response = {
  type t = {
    status: int,
    text: string,
  };

  let empty = {status: 200, text: ""};

  let send = (str, res) => {...res, text: str};
  let status = (status, res) => {...res, status};
};

/** Reresents the result of a middleware action */
type httpResult('a) =
  | CannotHandle
  | Done(Response.t)
  | Continue('a, Request.t);

type async('a) = (('a => unit, exn => unit)) => unit;
type asyncHttpResult('a) = async(httpResult('a));

module Handler = {
  type t('a) = (Request.t, Response.t) => asyncHttpResult('a);
  type m('a, 'b) = 'a => t('b);

  let (>>=) = (x: t('a), f: 'a => t('b)): t('b) =>
    (req, res) => (
      ((cb, err)) =>
        x(
          req,
          res,
          (
            fun
            | Continue(x, req) => f(x, req, res, (cb, err))
            | CannotHandle => cb(CannotHandle)
            | Done(x) => cb(Done(x)),
            err,
          ),
        ):
        asyncHttpResult('b)
    );

  let (>=>) = (f: m('a, 'b), g: m('b, 'c)): m('a, 'c) =>
    (x: 'a) => f(x) >>= g;

  let done_ = (response, (cb, _)) => cb(Done(response));
  let cannotHandle = ((cb, _)) => cb(CannotHandle);
  let continue = (data, req, _res, (cb, _)) => cb(Continue(data, req));
};

type middleware('a, 'b) = 'a => Handler.t('b);

let path = searchPath: middleware('a, 'a) =>
  (data, req, res) => {
    open Js;
    open Request;
    let requestPath = req |> getPath;
    let length = String.length(searchPath);
    let newPath = String.substr(~from=length, requestPath);
    Handler.(
      String.startsWith(searchPath, requestPath)
      && String.startsWith("/", requestPath) ?
        continue(data, {...req, path: newPath}, res) : cannotHandle
    );
  };

let sendText = (text, _, res, (cb, _)) =>
  cb(Done(Response.send(text, res)));

let rec router = (routes, data, req, res, (cb, err)) =>
  switch (routes) {
  | [] => cb(CannotHandle)
  | [route, ...routes] =>
    route(
      data,
      req,
      res,
      (
        fun
        | CannotHandle => router(routes, data, req, res, (cb, err))
        | x => cb(x),
        err,
      ),
    )
  };

let scanPath = (pattern, f, data, req) =>
  Scanf.sscanf(req |> Request.getPath, pattern, f, data, req);

let method = (m, x, req, res) =>
  Request.(Handler.(req.method == m ? continue(x, req, res) : cannotHandle));

let get = x => method("GET", x);

let post = x => method("POST", x);

let tryGetCookie = name: middleware('a, option(string)) =>
  (_, req, res) => Handler.continue(Request.getCookie(name, req), req, res);

let getCookie =
    (~onMissing: middleware('a, string), name): middleware('a, string) =>
  (x: 'a, req, res) =>
    switch (Request.getCookie(name, req)) {
    | Some(cookie) => Handler.continue(cookie, req, res)
    | None => onMissing(x, req, res)
    };

let createServer = (handleFunc: middleware('a, 'b)) =>
  (. req, res) => {
    module HttpResponse = NodeModules.Http.Response;
    let req = Request.from_native_request(req);
    let result = handleFunc((), req, Response.empty);
    result((
      fun
      | CannotHandle =>
        /* If the handler cannot handle the request, then doing nothing might
           be the most sensible thing to do, as other request handlers could have
           been attached to the http server. */
        ()
      | Continue(_) =>
        /* Should probably result in a HTTP 500 status - the server code is misconfigured */
        ()
      | Done(response) => {
          res |> HttpResponse.writeHead(response.status);
          res |> HttpResponse.end_(response.text);
        },
      _exn => {
        res |> HttpResponse.writeHead(500);
        res |> HttpResponse.end_("Internal server error");
      },
    ));
  };
