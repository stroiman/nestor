type async('a) = (('a => unit, exn => unit)) => unit;

module Request = {
  module Body = {
    type t = NodeModules.Http.Request.Body.t;

    let bodyString = (input: t, cb: string => unit): unit => {
      let on = NodeModules.Http.Request.Body.on;
      let data = ref("");
      input->on("data", d => {
        data := data^ ++ d;
        ();
      });
      input->on("end", _ => cb(data^));
    };

    let bodyStringA = (input: t): async(string) =>
      ((fs, _fe)) => {
        let on = NodeModules.Http.Request.Body.on;
        let data = ref("");
        input->on("data", d => {
          data := data^ ++ d;
          ();
        });
        input->on("end", _ => fs(data^));
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
  | Continue('a)
  | NewContext('a, Request.t, Response.t);

type asyncHttpResult('a) = async(httpResult('a));

module Async = {
  type t('a) = async('a);

  let return = (x: 'a): t('a) => ((fs, _fe)) => fs(x);

  let (>>=) = (x: t('b), f: 'b => t('a)): t('a) =>
    ((fs, fe)) => x((y => f(y, (fs, fe)), fe));
};

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
            | Continue(x) => f(x, req, res, (cb, err))
            | NewContext(x, req, res) => f(x, req, res, (cb, err))
            | CannotHandle => cb(CannotHandle)
            | Done(x) => cb(Done(x)),
            err,
          ),
        ):
        asyncHttpResult('b)
    );

  let (>>) = (x, y) => x >>= _ => y;

  let (>=>) = (f: m('a, 'b), g: m('b, 'c)): m('a, 'c) =>
    (x: 'a) => f(x) >>= g;

  let done_ = (response, (cb, _)) => cb(Done(response));
  let cannotHandle = ((cb, _)) => cb(CannotHandle);
  let continue = (data: 'a): asyncHttpResult('a) =>
    ((cb, _)) => cb(Continue(data));
  let newContext = (data: 'a, req, res): asyncHttpResult('a) =>
    ((cb, _)) => cb(NewContext(data, req, res));
};

type middleware('a, 'b) = 'a => Handler.t('b);

let path = (searchPath): Handler.t('a) =>
  (req, res) => {
    open Js;
    open Request;
    let requestPath = req |> getPath;
    let length = String.length(searchPath);
    let newPath = String.substr(~from=length, requestPath);
    Handler.(
      String.startsWith(searchPath, requestPath)
      && String.startsWith("/", requestPath)
        ? newContext((), {...req, path: newPath}, res) : cannotHandle
    );
  };

let sendText = (text, _, res, (cb, _)) =>
  cb(Done(Response.send(text, res)));

let createServer = (handleFunc: Handler.t('b)) =>
  (. req, res) => {
    module HttpResponse = NodeModules.Http.Response;
    let req = Request.from_native_request(req);
    let result = handleFunc(req, Response.empty);
    result((
      fun
      | CannotHandle =>
        /* If the handler cannot handle the request, then doing nothing might
           be the most sensible thing to do, as other request handlers could have
           been attached to the http server. */
        ()
      | NewContext(_)
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

/**
  * Contains components for building a handler/web application, e.g. routers, method filters,
  * cookie extraction, body parsing, etc.
  */
module Middlewares = {
  let rec router = (routes): Handler.t('a) =>
    (req, res, (cb, err)) =>
      switch (routes) {
      | [] => cb(CannotHandle)
      | [route, ...routes] =>
        route(
          req,
          res,
          (
            fun
            | CannotHandle => router(routes, req, res, (cb, err))
            | x => cb(x),
            err,
          ),
        )
      };

  let scanPath = (pattern, f): Handler.t('a) =>
    (req, res) => Scanf.sscanf(req |> Request.getPath, pattern, f, req, res);

  let method = (m, req, _res) =>
    Request.(Handler.(req.method == m ? continue() : cannotHandle));

  let get: Handler.t(unit) = method("GET");

  let post: Handler.t(unit) = method("POST");

  let tryGetCookie = (name): Handler.t(option(string)) =>
    (req, _res) => Handler.continue(Request.getCookie(name, req));

  let getCookie = (~onMissing: Handler.t(string), name): Handler.t(string) =>
    (req, res) =>
      switch (Request.getCookie(name, req)) {
      | Some(cookie) => Handler.continue(cookie)
      | None => onMissing(req, res)
      };
};
