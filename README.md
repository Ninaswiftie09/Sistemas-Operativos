# Laboratorio 2.2

## Problema asignado 
Dining Philosophers - N-1 filósofos → C (pthreads)

### De que se trata el problema 
El problema de Dining Philosophers es un ejercicio clásico de sincronización en sistemas operativos. Hay N filósofos sentados en círculo y entre cada par hay un tenedor (N tenedores en total). Cada filósofo alterna entre “pensar” y “comer”, pero para comer necesita dos tenedores: el de la izquierda y el de la derecha.

El problema principal es la concurrencia: si varios filósofos intentan comer al mismo tiempo, pueden ocurrir errores de sincronización. En especial, puede aparecer un deadlock si todos toman un tenedor y se quedan esperando el segundo indefinidamente, dejando el sistema trabado. También puede existir starvation, donde algún filósofo se queda esperando por mucho tiempo porque otros se le adelantan.

Una solución común es la variante N−1 filósofos, que limita a máximo N−1 filósofos intentando comer al mismo tiempo (por ejemplo, con un semáforo). Esto rompe la condición circular que causa el deadlock y garantiza que al menos uno pueda conseguir dos tenedores, comer y liberar recursos para que el resto avance.