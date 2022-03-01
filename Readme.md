# IPK - 1. projekt - Jednoduchý HTTP server

## Autor
Vojtěch Dvořák (xdvora3o)

## O projektu
Cílem projektu bylo implementovat jednoduchý HTTP server poskytující několik základních informací o systému.

## Použití

### Strana serveru
Nejprve přeložíme zdrojové kódy pomocí přiloženého `Makefile` a programu `make` příkazem

```
make
```

Překlad je proveden pomocí překladače `gcc` (je nutné, aby byl v systému dostupný). Server poté je možné spustit pomocí

```
./hinfosvc ČÍSLO_PORTU
```

nebo pro běh na pozadí

```
./hinfosvc ČÍSLO_PORTU &
```

Kde ČÍSLO_PORTU je číslo dostupného portu na cílové platformě, kde má HTTP server naslouchat. V případě, že číslo portu není validní (nebo dojde k jiné chybě např. při čtení souboru) je vypsána chybová hláška na standardní chybový výstup a program je ukončen s chybovým kódem 1.
Program je po spuštění možné ukončit zasláním signálu SIGINT (nebo pomocí `CTRL-C`).

### Strana klienta
Se serverem je možné komunikovat (po jeho zprovoznění) pomocí prohlížeče nebo prostřednictvím nástroje `wget` či `curl`. Informace ze serveru si můžeme vyžádat adresou ve tvaru http://JMÉNO_SERVERU:ČÍSLO_PORTU/CESTA, kde JMÉNO_SERVERU je jméno serveru, na kterém je projekt spuštěn, a ČÍSLO_PORTU, kde tento server naslouchá. CESTA pak může být právě jeden z těchto řetězců:

+ **hostname** vrátí síťové jméno počítače se serverem včetně domény  

+ **cpu-name** vrátí informaci o procesoru

+ **load** vrátí aktuální zatížení procesoru (v procentech)

HTTP žádost by měla být ve validním formátu a použita by měla být metoda `GET`. V opačném případě je vrácena HTTP odpověď `400 Bad Request`. V případě, že je CESTA neodpovídá žádné výše pospané možnosti, je vrácena odpověď s kódem `404 Not Found`. Jinak je navrácena HTTP odpověď `200 OK`.

## Příklady
Výpis síťového jména počítače:
```
./hinfosvc 12345 &
curl localhost:12345/hostname
```

Výpis jména procesoru:
```
./hinfosvc 12345 &
curl localhost:12345/cpu-name
```

Výpis zatížení CPU v procentech:
```
./hinfosvc 12345 &
curl localhost:12345/load
```


## Popis implementace
Nejprve je pomocí standardních knihovních funkcí vytvořen a nastaven socket, na kterém server naslouchá. Poté server čeká ve smyčce `while` na žádost. Po jejím obdržení odešle požadovaná data na získanou klientskou adresu (s příslušnou HTTP hlavičkou).
Síťové jméno je čteno ze souboru `/etc/hostname`, jméno procesoru z `/proc/cpuinfo` a zatížení je vypočítáno pomocí hodnot ze souboru `/proc/stat`. Pro výpočet zatížení je použit vzorec ze stránky https://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux. 
Jelikož se má jednat o lightweight server, jsou pro vytváření HTTP odpovědi a pro načítání dat ze souboru využity staticky alokované buffery. Vzhledem k charakteru odesílaných dat (maximálně stovky B pro HTTP odpovědi včetně hlavičky) je jejich velikost dostatečná (2KiB). Pokud by mělo dojít k jejich přetečení, data jsou oříznuta na velikost použitých bufferů.

