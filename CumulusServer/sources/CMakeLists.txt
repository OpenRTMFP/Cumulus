SET(OBJECTS FileWatcher
	LUABroadcaster
	LUAByteReader
	LUAByteWriter
	LUAClient
	LUAClients
	LUAFlowWriter
	LUAGroup
	LUAGroups
	LUAInvoker
	LUAListener
	LUAListeners
	LUAMail
	LUAMember
	LUAPublication
	LUAPublications
	LUAQualityOfService
	LUAServer
	LUAServers
	LUATCPClient
	LUATCPServer
	LUAUDPSocket
	Script
	Server
	ServerConnection
	ServerMessage
	Servers
	Service
	SMTPSession
	TCPClient
	TCPServer
	UDPSocket
)

ADD_LIBRARY(libCumulusServer ${OBJECTS})

SET(DEPS
	${POCO_NET}
	${POCO_UTIL}
	${POCO_XML}
	${POCO_FOUNDATION}
	${OPENSSL_LIBRARY}
	${LUAJIT_LDFLAGS}
)

ADD_EXECUTABLE(CumulusServer main.cpp)

TARGET_LINK_LIBRARIES( CumulusServer libCumulusServer CumulusLib ${DEPS})

# vim:ft=cmake
