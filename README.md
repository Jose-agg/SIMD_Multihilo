# Uso de instrucciones SIMD y programación multihilo #

Proyecto de investigación sobre la reducción en el tiempo de ejecución de aplicaciones con el uso de instrucciones SIMD y multihilo. Para ver la lista completa de instrucciones SIMD que soportan los procesadores Intel, visite el siguiente [enlace](https://software.intel.com/sites/landingpage/IntrinsicsGuide/)

Para una explicación detallada del funcionamiento de estas instrucciones y su aplicación a este proyecto, revise los documentos:
- [Monohilo: instrucciones comunes y SIMD](https://github.com/MrKarrter/SIMD_Multihilo/blob/master/Documentacion/Fase%20Monohilo.pdf)
- [Multihilo: instrucciones comunes y SIMD](https://github.com/MrKarrter/SIMD_Multihilo/blob/master/Documentacion/Fase%20Multihilo.pdf)

Los resultados de la ejecución del proyecto se encuentran en el archivo [Tiempos](https://github.com/MrKarrter/SIMD_Multihilo/blob/master/Documentacion/Tiempos.xlsx). En el caso de las versiones multihilo, el proyecto detecta el máximo número de hilos que soporta la maquina anfitrión para ejecutar de forma paralela. El número de hilos, en la maquina donde se tomaron los tiempos, es 8.

Por último, esta es una comparativa del uso de la CPU frente al tiempo empleado a la hora de ejecutar el proyecto. De izquierda a derecha serian:
- Aplicación con operaciones comunes monohilo.
- Aplicación con operaciones SIMD monohilo.
- Aplicación con operaciones comunes multihilo.
- Aplicación con operaciones SIMD multihilo.

![comparativa](https://github.com/MrKarrter/SIMD_Multihilo/blob/master/Documentacion/Captura%20Total%20Tiempos.jpg)

# Autoría #

Este proyecto fue desarrollado por:
- Cristóbal Solar Fernández.
- Hugo Leite Pérez.
- José Antonio García García.

