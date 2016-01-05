# Distributed Shared Memory
Projet de Système et Réseau - Implémentation d'une Mémoire Partagée Distrubée 

*Thibaut Gourdel*, *Rémi Perrot*

Enseirb-Matmeca, T1-G1 - 2015

## Phase 1 : Lancement des processus

Le programme `dsmexec` s'occupe de lancer des processus sur toutes les machines indiquées dans `machine_file`. Sur les machines distantes, `dsmwrap` est utilisé, pour établir la connexion avec le lanceur (`dsmexec`).
Suite à un échange d'informations lanceur / lancé, les programmes sur les machines distantes se connectent entre eux.

## Phase 2 : Mise en place de la mémoire partagée

Cette fois, `dsmwrap` est remplacé directement par le fichier `dsm`. De cette façon, dans le programme lancé sur les machines distantes, il suffit d'inclure la librairie avec `#include "dsm.h"`.
Ensuite, deux fonctions seront utilisées :

* `dsm_init(argc,argv)`
    * renvoie le pointeur vers le début de la mémoire partagée
    * effectue les connexions entre programmes
    * mets en place le traitant de signal pour les segmentations faults
    * lance le démon d'écoute (pour les échanges de pages de mémoire)
* `dsm_finalize()`, qui ferme les sockets et libère les structures.
