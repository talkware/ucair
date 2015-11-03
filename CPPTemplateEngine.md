# Introduction #

Anyone who has ever written embedded HTML in their C++/Perl programs can tell you how ugly and unmanageable the code can become. Luckily there are [web page template engines](http://en.wikipedia.org/wiki/Template_engine_%28web%29) that allows one to separate the presentation logic (HTML pages) from application logic (data & algorithms). Examples include JSP for Java and [Genshi](http://genshi.edgewall.org) for Python. But for C++, there is no really good choice: [Google's ctemplate](http://code.google.com/p/google-ctemplate/) is overly simple (it provides no control logic such as branching and repeating). [teng](http://teng.sourceforge.net) is more powerful (of course, still much inferior to JSP/Genshi due to C++'s limitations), but I want something simpler, easier, and under my full control :) So I came up with this small two-file library, though I must say the inspiration largely comes from teng.

# Template File Syntax #

The enire template file must be enclosed in a pair of `<template:main>` tags:

```
<template:main>
...
...
</template:main>
```

### Variable Replacement ###

`${VARIABLE_NAME}`


Example:
```
<title>${my_title}</title>
```

Here `${my_title}` will be replaced by the value of the `my_title` variable, which is passed to the template engine at run time. Note that only string variables are supported. If you pass in "1 + 1", it will be interpreted as a string of length 5, instead of number 2.

### If Clause ###

```
<template:if name="VARIABLE_NAME" value="VALUE" test="TEST">
...
...
</template:if>
```

The enclosed content will be output only if the test returns true for the variable. Note that `VALUE` is string only. If you set it to be "1 + 1", it will be interpreted as a string of length 5, instead of number 2.

`TEST` can take one of the following values:
  * `eq`: returns true if the variable value equals the designated `VALUE` (by string comparison)
  * `neq`: opposite of `eq`
  * `exist`: returns true if the variable exists (passed in) during run time. The `value=` section can be omitted.
  * `nexist`: opposite of `exist`
  * `empty`: return true if the variable does not exist or is an empty string. The `value=` section can be omitted.
  * `nempty`: opposite of `empty`

### Switch Clause ###

```
<template:switch name="VARIABLE_NAME">
  <template:case value="VALUE_1">
  ...
  </template:case>
  <template:case value="VALUE_2">
  ...
  </template:case>
  ...
  ...
  <template:default>
  ...
  </template:default>
</template:switch>
```

If any `case` clause matches the variable value, then the content inside that case is output, otherwise, the `default` clause gets output.

### For Clause ###

```
<template:foreach name="CHILD_NAME">
...
</template:foreach>
```

The enclosed content will be repeated for each "child" having the specified name (detailed below).

# Template Data Model #

Template data (to be rendered by a template) is represented as a hierarchy of name-value mappings:
  * Each node in the hierarchy contains a mapping from variable names to variable values (only string type is supported).
  * Each node may have child nodes that contain separate name-value mappings.
  * Child node inherits its parent's mapping: if a variable isn't found in the child's mapping, it is looked up in the parent's mapping, then the grandparent's mapping, and so on.
  * Child nodes have names. Child nodes sharing the same name are repeated in a `foreach` clause.

Now suppose we have the following template data in JSON representation:
```
{
  "query" : "Amazon",
  "language" : "English",
  "result" : [
               {
                 "title" : "Amazon.com",
                 "url" : "www.amazon.com"
               },
               {
                 "title" : "Amazon.cn",
                 "url" : "www.amazon.cn",
                 "language" : "Chinese"
               }
             ]
}
```

In our representation, it will have three nodes. The top level node has the mapping for "query" and "language", while the next level has two nodes, both having the mapping for "title" and "url" (but with different values). The two child nodes have the same name "result".

So if the template is like this
```
<template:main>
<html>
<title>${query}</title>
<body>
  <template:foreach name="result">
    <p><a href="${url}">${title}</a>
    ${language}</p>
  </template:foreach>
</body>
</html>
</template:main>
```

The results will be
```
<html>
<title>Amazon</title>
<body>
    <p><a href="www.amazon.com">Amazon.com</a>
    English</p>
    <p><a href="www.amazon.cn">Amazon.cn</a>
    Chinese</p>
</body>
</html>
```

Observations:
  * The variables are all replaced by the values in the data mappings.
  * Because the top level node has two child nodes named "result", the `foreach` subtemplate are repeated twice, being applied to each child node.
  * In the first child node, "language" is inherited from the parent, while in the second child node, "language" is defined in its own mapping.

# C++ API #

[template\_engine.h](http://code.google.com/p/ucair/source/browse/trunk/UCAIR09/template_engine.h) [template\_engine.cpp](http://code.google.com/p/ucair/source/browse/trunk/UCAIR09/template_engine.cpp)

```
class TemplateData:
TemplateData& set(const std::string &name, const std::string &value, bool xml_escape = true);
bool get(const std::string &name, std::string &value) const;
TemplateData addChild(const std::string &name);

class TemplateEngine:
TemplateEngine(const std::string &source_dir = "", bool detect_changes = true);
std::string render(const TemplateData &data, const std::string &source_file_name);

global function:
TemplateEngine& getTemplateEngine();
```

Notes:
  * Both `TemplateData::set` and `addChild` returns a `TemplateData` object (or self-reference), so you can chain operations.
  * `TemplateData::set` does XML entity escaping by default
  * `TemplateEngine`'s constructor has options to detect changes in the template file. Thus you can change the template and observe the effect without restarting the program.
  * You can get the `TemplateEngine` singleton using `getTemplateEngine`

To render the above example, the code is
```
TemplateData t_main;
t_main.set("query", "Amazon").set("language", "English");
t_main.addChild("result").set("title", "Amazon.com").set("url", "www.amazon.com");
t_main.addChild("result").set("title", "Amazon.cn").set("url", "www.amazon.cn").set("lanauge", "Chinese");
string rendered_html = getTemplateEngine().render(t_main, "example_template_file.htm");
```