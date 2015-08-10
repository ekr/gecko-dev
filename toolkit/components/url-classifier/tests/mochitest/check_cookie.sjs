
function handleRequest(request, response)
{
    var host;

    dump("Check cookie script invoked");

    // avoid confusing cache behaviors
    response.setHeader("Cache-Control", "no-cache", false);
    response.setHeader("Content-Type", "text/plain", false);
    response.setHeader("Access-Control-Allow-Origin", "*", false);

    // used by test_user_agent tests
    //response.write(request.getHeader('User-Agent'));
    try {
        request.getHeader("Cookie");
        response.write("");
    } catch (e) {
        response.setStatusLine("1.0", 400, "Cookie required");
    }
}
