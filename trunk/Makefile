CXX = g++

MXMLDIR = /usr/local
SQLITEDIR = /usr/local
BOOSTDIR = /mounts/times2/disks/0/local/boost-1.42

CXXFLAGS = -I$(MXMLDIR)/include -I$(SQLITEDIR)/include -I$(BOOSTDIR)/include -O3 -Wall

LDFLAGS = -L$(MXMLDIR)/lib -L$(SQLITEDIR)/lib -L$(BOOSTDIR)/lib -lboost_thread -lboost_system -lboost_program_options -lboost_regex -lboost_filesystem -lboost_signals -lmxml -lsqlite3

VPATH = UCAIR09

OBJS = adaptive_search_ui.o agglomerative_clustering.o algorithm.o all_components.o aol_wrapper.o basic_search_ui.o common_util.o component.o config.o connection.o connection_manager.o console_ui.o delayed_signal.o doc_stream_manager.o doc_stream_ui.o document.o exe_main.o http_download.o index_manager.o index_util.o logger.o log_importer.o long_term_history_manager.o long_term_search_model.o main.o mixture.o page_module.o porter.o properties.o prototype.o reply.o request.o request_parser.o reranking_list_view.o result_list_view.o rss_feed_parser.o search_engine.o search_history_ui.o search_menu.o search_model.o search_model_widget.o search_proxy.o search_topics.o search_topics_ui.o server.o session_widget.o simple_index.o sqlitepp.o static_file_handler.o template_engine.o template_engine_wrapper.o test_main.o ucair_server.o ucair_util.o url_components.o url_encoding.o user.o user_event.o user_manager.o user_search_record.o value_map.o xml_dom.o xml_util.o yahoo_boss_api.o yahoo_search_api.o

PROG = ucair

$(PROG): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJS) $(PROG)
