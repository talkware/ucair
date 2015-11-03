#New UCAIR Java Implementation

This is a complete rewrite of the original UCAIR C++ implementation in Java that leverages mature technologies including JSP/JSTL and Spring IOC/MVC. It has a more modern modular architecture and offers a cleaner starting place for adding new personalized search features.

# Running #

Steps:
  1. Make sure you have [Java 7](http://java.com/en/download/index.jsp)
  1. Make sure the "java" command is on your system path. [How do I set or change the PATH system variable?](http://java.com/en/download/help/path.xml)
  1. Download [ucair-java-bin-2013-05-08.zip](http://code.google.com/p/ucair/downloads/detail?name=ucair-java-bin-2013-05-08.zip&can=2&q=) and unpack.
  1. Run start.bat
  1. Point your browser to http://localhost:10816

Notes:
  1. The first few page loads may be slow due to one-time JSP compilation cost
  1. In IE, you may need to [enable compatibility mode](http://windows.microsoft.com/en-us/internet-explorer/use-compatibility-view#ie=ie-10) if you see broken layout. Firefox and Chrome run fine.
  1. Source can be downloaded [here](http://code.google.com/p/ucair/downloads/detail?name=ucair-java-src-2013-05-08.zip&can=2&q=) or viewed on [github](https://github.com/talkware/UCAIR/tree/master/UCAIR)

# What's included in this release #
  * [Bing Search API](https://datamarket.azure.com/dataset/5BA839F1-12CE-4CCE-BF57-A49D98D29A44) integration (you may need to generate your own Bing account key)
  * Single-query click-based implicit feedback

# What's not included in this release #
  * Session-scope implicit feedback
  * Long-term implicit feedback
  * Persisting search log to database
  * Search keyword match highlighting
  * Windows installer