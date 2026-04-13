#ifndef __ERR_MSG_H__
#define __ERR_MSG_H__

char *mensaje_err[][2]= { 
{ "Programa finalizado satisfactoriamente",         "Program finished OK" },
{ "Error en número de argumentos",         "Wrong number of arguments" },
{ "Protocolo ICMP desconocido",                   "Unknown ICMP protocol" },
{ "No puedo crear socket tipo RAW",             "Can't build RAW sockets" },
{ "Tipo de paquete ICMP falta o es incorrecto",
                                  "Incorrect or missing ICMP packet type" },
{ "Gateway incorrecto",                                   "Wrong gateway" }, 
{ "Destino de ruta falta o es incorrecto",
                                 "Incorrect or missing route destination" }, 
{ "Código de paquete ICMP falta o es incorrecto", 
                                  "Incorrect or missing ICMP packet code" },
{ "Host original incorrecto",                       "Wrong original host" },
{ "Error enviando paquete",                  "Error while sending packet" },
{ "Protocolo todavía no implementado",   "Protocol still not implemented" },
{ "Dirección IP o host de spoof incorrecto", "Wrong spoof IP or hostname" },
{ "No pude reservar memoria para unión data_hdr", 
                                   "Can't allocate union data_hdr memory" },
{ "Dirección IP o host destino falta o es incorrecto",
                            "Missing or wrong destination IP or hostname" },
{ "Protocolo desconocido",                             "Unknown protocol" },
{ "No puedo crear socket de lectura RAW",   "Can't build read RAW socket" },
{ "Error leyendo socket RAW",            "Error while reading RAW socket" },
{ "Error al iniciar manejador de señal", 
                                "Error while initializing signal handler" },
{ "Patrón de datos demasiado grande",
                                "Data pattern too big. Don't be cruel :)" },
{ "Puerto origen incorrecto",               "Incorrect source port value" },
{ "Puerto destino incorrecto",         "Incorrect destination port value" },
{ "Valor de timeout incorrecto",                "Incorrect timeout value" },
{ "Echo ID incorrecto",                         "Incorrect Echo ID value" },
{ "Número de secuencia incorrecto",        "Incorrect sequence number" },
{ "Datos de Echo incorrectos",                "Incorrect Echo data value" },
{ "Error en IP_HDRINCL",                  "IP_HDRINCL error"},
{ "Direccion de router incorrecta",            "Incorrect router address" },
{ "Bytes basura incorrectos",                    "Incorrect data garbage" },
{ "Puntero incorrecto",                         "Incorrect pointer value" }
                        };

#endif
