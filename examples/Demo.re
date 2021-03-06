open Index;
open Index.Handler;
open Index.Middlewares;

let delay = (_req, _res, (cb, _)) =>
  Js.Global.setTimeout(() => cb(Continue(())), 1000) |> ignore;

let server =
  router([
    path("path") >> ( sendText("Hello from /path")),
    path("delay") >> delay >> ( sendText("This renders after 1 second")),
    sendText("Not found"),
  ])
  |> createServer;

%raw
{|
const http = require('http');
let httpServer = http.createServer(server);
httpServer.listen(4000);
|};
