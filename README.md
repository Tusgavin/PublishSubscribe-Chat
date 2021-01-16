# PublishSubscribe-ClientServer
A simple server and client using Publish/Subscribe paradigm


## Commands
To compile the program, to src/ directory and run the command:

```properties 
make 
```

In order to run the server, run the command:

```properties
./servidor <port> 
```

In order to run the client, open another Terminal window and run the command:

```properties
./cliente <IPv4-address> <server-port> 
```

## Publish / Subscribe Paradigm
### Subscribing
To subscribe to a tag, type the flag '+' and the tag

```properties
+programming
```

### Unsubscribing
To unsubscribe to a tag, type the flag '-' and the tag

```properties
-flutter
```

### Broadcast Message
To broadcast a message to all subscribers of a tag include the flag '#' and the tag. In order to recogonize it as a broadcast message, it must have a space character before the '#' and a space character or be the end of a message.

```properties
very cool programming language #Golang
```

### Kill the program
From any client, sending the message '##kill' will terminate the server and all clients.

```properties
##kill
```
