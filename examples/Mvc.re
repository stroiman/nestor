open Index;

module Async = {
  type t('a) = ('a => unit) => unit;

  let return = (x: 'a): t('a) => (fn: 'a => unit) => fn(x);
  let (>>=) = (x: t('a), f: 'a => t('b)): t('b) => cb => x(a => f(a, cb));
  let (>>|) = (x: t('a), f: 'a => 'b): t('b) => cb => x(a => cb(f(a)));
};

module Repository = {
  type user = {firstName: string};

  let findAll = (): Async.t(list(user)) =>
    Async.return([{firstName: "John"}]);

  let find = _userId: Async.t(user) => Async.return({firstName: "John"});
};

module Converters = {
  let usersToJson = (_u: list(Repository.user)) => "\"json\"";
  let userToJson = (_u: Repository.user) => "\"json\"";
};

let getUsers = ((), _req, res) =>
  Async.(
    Repository.findAll()
    >>| (users => Converters.usersToJson(users))
    >>= (json => sendJson(json, res))
  );

let getUser: middleware(string, unit) =
  (userId: string) => sendText("Foo" ++ userId);

let scanUserId: middleware(unit, string) =
  scanPath(
    "/%s",
    (
      userId,
      _a,
      req,
      _res,
      /* sendText(userId, a, req, res) */
      cb,
    ) =>
    cb(Continue(userId, req))
  );

let getU: middleware(unit, unit) = get;

let foo: middleware(unit, unit) = Index.Handler.(scanUserId >=> getUser);

module UsersApi = {
  open Index.Handler;
  let router: middleware(unit, unit) =
    Index.router([
      getU >=> scanUserId >=> getUser,
      get >=> path("/") >=> getUsers,
      get
      >=> scanPath("/%s", (userId, _, a, req, res) =>
            getUser(userId, a, req, res)
          ),
      get
      >=> scanPath(
            "/%s",
            (
              userId,
              _a,
              _req,
              res,
              /* sendText(userId, a, req, res) */
              cb,
            ) =>
            cb(Done(Response.send(userId, res)))
          ),
    ]);
};

let requestHandler =
  Index.Handler.(router([path("/users") >=> UsersApi.router]))
  |> createServer;
