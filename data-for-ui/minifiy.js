var minify = require('html-minifier').minify;
var fs = require('fs')
var options = {
    removeAttributeQuotes: true,
    minifyCSS: true,
    minifyJS: true,
    // removeAttributeQuotes: true,
    // removeEmptyAttributes: true,
    // removeEmptyElements: true,
    // removeComments: true,
    // collapseBooleanAttributes: true,
    collapseWhitespace: true,
    collapseBooleanAttributes: true,
    collapseWhitespace: true,
    decodeEntities: true,
    removeAttributeQuotes: true,
    removeComments: true,
    removeEmptyAttributes: true,
    removeEmptyElements: true,
    removeOptionalTags: true,
    removeRedundantAttributes: true,
    removeScriptTypeAttributes: true,
    removeStyleLinkTypeAttributes: true,
    removeTagWhitespace: true
};

fs.readFile('./ui.html', function(err, html) {
    if (err) {
        throw err;
    }
    // console.log(html.toString());
    var output = minify(html.toString(), options);
    // console.log(output);
    fs.writeFile('./ui.min.html', output, err => {
        if (err) {
            console.error(err)
            return
        }
        console.log("Successfully Written.");
    });
});