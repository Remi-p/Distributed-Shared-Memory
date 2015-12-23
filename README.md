# Distributed Shared Memory
Projet de Système et Réseau - Implémentation d'une Mémoire Partagée Distrubée 

*Thibaut Gourdel*, *Rémi Perrot*

Enseirb-Matmeca, T1-G1 - 2015

## Phase 1 : Lancement des processus

Le programme `dsmexec` s'occupe de lancer des processus sur toutes les machines indiquées dans `machine_file`. Sur les machines distantes, `dsmwrap` est utilisé, pour établir la connexion avec le lanceur (`dsmexec`).
Suite à un échange d'informations lanceur / lancé, les programmes sur les machines distantes se connectent entre eux.

## Phase 2 : Mise en place de la mémoire partagée
