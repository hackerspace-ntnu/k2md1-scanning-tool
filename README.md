# k2md1-scanning-tool

## Prerequisites
* Kinect v2
* Linux
* Kontorstol eller noe annet som kan rotere

## How to 3D scan

1. På [releases-siden](https://github.com/hackerspace-ntnu/k2md1-scanning-tool/releases) under "Assets", last ned `ScanTheThing.AppDir.tar.xz`, og pakk ut innholdet.

2. Pass på at riktige paths gis til programmet (`ScanTheThing`). Dette kan f.eks. gjøres gjennom et shell script kalt `ScanTheThing.sh` (plassert i samme mappe) med følgende innhold:
   ```bash
   #!/bin/bash
   export APPDIR=$(dirname $0)
   export LD_LIBRARY_PATH=$(dirname $0)/lib
   $(dirname $0)/ScanTheThing
   ```
   Pass også på at `ssd_recon` (under `bin/`) har permission til å execute.

3. Kjør `sudo ./ScanTheThing.sh`<sup id="a-sudo">[**1**](#f-sudo)</sup>

4. Fjern alle ting fra området foran Kinecten og trykk på "Calibrate"<sup id="a-calibrate">[**2**](#f-calibrate)</sup> og så "Stop calibration" etter det har dukket opp et vindu med et svart-hvitt-bilde av rommet.

5. Sett personen som skal scannes på en kontorstol så nærme som mulig foran Kinecten, og sørg for at hen er rett i ryggen og har ryggsøylen rett over rotasjonspunktet - slik at det blir minst mulig slingring når hen snurrer rundt.

6. Sørg for at personen har øynene lukket<sup id="a-eyes-closed">[**3**](#f-eyes-closed)</sup> og bakhodet<sup id="a-back-of-head">[**4**](#f-back-of-head)</sup> mot Kinecten før scanningen starter, og trykk på "Scan".

7. Enten få personen til å snurre en hel rotasjon ved hjelp av beina, eller få en annen person til å snurre stolen - med så jevn fart som mulig; det er ideelt å ta mellom 200 og 300 bilder<sup id="a-num-images">[**5**](#f-num-images)</sup> (antallet står i den horisontale baren nederst i vinduet til programmet).

8. Trykk på "Stop scan" og så "Reconstruct", og vent til det dukker opp et vindu med en punktsky av scannen (bevegelseskontroller i fotnote <sup id="a-controls">[**6**](#f-controls)</sup>).

9. Trykk på **Enter** slik at punktsky-vinduet lukker seg, og vent på at punktskyen gjøres om til en 3D-modell bestående av polygoner; dette kan ta noen minutter (først en progress-bar i programvinduet, og så tekst i terminalen som sier progresjonen i prosent).

10. Når det dukker opp en dialogboks som sier "Finished reconstruction!", er 3D-modellen `finally.ply` ferdig generert, og den ligger i working directory<sup id="a-generated-files">[**7**](#f-generated-files)</sup>. Denne kan nå importeres i f.eks. Blender for å manuelt fjerne artifacts og pusse opp modellen, og så eksporteres til STL-filformat for å printes.

11. Lukk programmet helt ("OK"-knappen gjør ingenting) og gjenta fra steg 3 for neste scan.


### Footnotes
> <sup id="f-sudo">1</sup> Med `sudo` slik at programmet får tilgang til å bruke Kinecten. [↩](#a-sudo)
>
> <sup id="f-calibrate">2</sup> Dette gjør at programmet vet hva som skal ikke skal være med i scannen. [↩](#a-calibrate)
>
> <sup id="f-eyes-closed">3</sup> Ser ofte bedre ut på den ferdige scannen. [↩](#a-eyes-closed)
>
> <sup id="f-back-of-head">4</sup> Her blir "sømmen" som programmet produserer fra starten og slutten på scanningen, som ser bedre ut på bakhodet, fordi det er mindre synlig der sammenligna med hvis sømmen f.eks. hadde vært i ansiktet. [↩](#a-back-of-head)
>
> <sup id="f-num-images">5</sup> Hvis det er fler enn 300 tar det fort veldig lang tid å gjøre etterprosesseringen. [↩](#a-num-images)
>
> <sup id="f-controls">6</sup> **WASD** og mus (hold inne venstreklikk og dra for å se rundt) for FPS-kontroller, **Q**/**E** for å bevege opp/ned, og **Space** for å toggle mellom høyere og lavere bildekvalitet. [↩](#a-controls)
>
> <sup id="f-generated-files">7</sup> Sammen med noen hundre `.ppm`-bildefiler og en tekstfil kalt `tmp_views.txt`, som programmet har lagd underveis; disse kan slettes, men de vil uansett bli overskrevet under neste scan. [↩](#a-generated-files)
