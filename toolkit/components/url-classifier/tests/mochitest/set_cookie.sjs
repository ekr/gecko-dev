
function handleRequest(request, response)
{
    // avoid confusing cache behaviors
    response.setHeader("Cache-Control", "no-cache", false);
    response.setHeader("Content-Type", "text/plain", false);
    response.setHeader("Access-Control-Allow-Origin", "*", false);
    response.setHeader("Set-Cookie", "Test-cookie");
    response.write("");
}

