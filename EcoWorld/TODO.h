#ifndef DEF_TODO
#define DEF_TODO

/*
TODO.h : Liste des tâches à faire, ne pas inclure
*/

/*
BUGS Connus :
-placer les plus grandes res au début ; ne pas les sélectionner lors de la création du menu options
- BUG : Echap dans le menu options avec la fenêtre de confirmation, puis retour dans le menu options
- BUG : Les versions 64 bits compilent mais plantent !
- Problème : les tests ne se lancent pas en mode DEBUG

- BUG : Crash du jeu lors du défilement de la fenêtre d'informations avec la molette de la souris : Bug de la fonction DispatchMessage dans CIrrDeviceWin32::run() !

- BUG : Lors du chargement d'une partie (sûrement aussi lors des parties normales), la sélection de bâtiments ne sélectionne pas les bons nodes
- BUG lié : Lors de la sélection d'un bâtiment grâce à son tour de construction,
	si c'est la face arrière du tour de construction qui est visée (visible car le backface culling est désactivé),
	c'est un autre bâtiment (node précédent dans la liste des nodes) qui est sélectionné (BUG d'Irrlicht sûrement)
	=> Ces bugs viennent d'un bug dans la détermination d'un node d'après un triangle lui appartenant
- ==> BUG : La sélection d'un bâtiment grâce à son bloc de béton ne fonctionne pas

- BUG : Dans le shader de l'eau, les parties des bâtiments en-dessous de l'eau ne sont plus bien alignés avec les parties au-dessus de l'eau, et les parties en-dessous de l'eau ne sont pratiquement plus visibles !

- BUG : Les blocs de béton disparaissent parfois du rendu lorsqu'ils sont sur le coin de l'écran (mais toujours visibles) et proches de la caméra (en mode caméra FPS)

- BUG : Les ombres stencil ne fonctionnent plus du tout : elles ne fonctionne qu'avec l'eau avec shaders, et font même bugger l'affichage de la GUI du jeu

- BUG : Chargement d'un terrain par double-clic (parties sauvegardées aussi ?) : Lorsque le terrain à charger a le même nom qu'un terrain déjà existant dans le dossier de données du jeu, ce n'est pas le terrain choisi qui est chargé, mais celui présent par défaut dans le dossier des données

- BUG : Lorsqu'il pleut et que le nom au-dessus d'un bâtiment est affiché, parfois la pluie perd sa transparence (ça le fait aussi parfois avec la fumée) (avec le driver DirectX 9, peut-être avec d'autres driver)

- BUG : Différentes incohérences lorsque le programme EcoWorld est lancé "tout seul" (sans DLL à côté ni dossier de données)
	-> Il semblerait y avoir des problèmes de chevauchement mémoire dans ce cas là ! (flagrant avec les hauteurs du terrain, qui sont complètement aléatoires ; ainsi qu'avec la résolution, qui a du mal à passer en 800x600 : 16 bits même après plusieurs clics sur le boutons appliquer)

- BUG : Lors du redimensionnement de la fenêtre de notes, les scroll bar ne sont pas automatiquement bien replacées

- BUG : Le pourcentage de production désiré de la centrale à charbon (et peut-être des autres usines) est mal chargé depuis les sauvegardes
		+ La position de la barre du pourcentage de production dans la fenêtre d'informations n'est pas réactualisée lorsque la sélection change de bâtiment !
- BUG : La scroll bar de la fenêtre d'informations ne se remet pas automatiquement bien, au changement de bâtiment (sélection/destruction) (elle ne se remet pas en haut lorsqu'il y a la place pour afficher tout le texte)

- BUG : Lors du choix d'options non supportées par la machine actuelle + avec activation de l'audio :
			Le jeu reviens aux options précédentes (l'audio redevient donc théoriquement désactivé), mais l'audio reste activé !
		=> BUG à revérifier !

- BUG : Petite coupure dans la texture des grilles de construction (sur la droite du panneau, tout au bord de la texture)



- BUG : Le soleil ne fait pas un tour par an, MAIS est à la bonne place lors du chargement d'une partie
	De plus, sa position ne concorde pas avec la direction de la lumière du soleil (SAUF lors du chargement d'une partie)
	=> Semble être résolu (à revérifier)

- BUG : Le rendu est buggé en mode OpenGL (les textures sont grises !) (sous le Toshiba Vista seulement : il fonctionne sur le nouveau Asus)

- BUG : (Sur ACER : Apparu lorsque le nombre de FPS < 10) : Les arbres perdent leur transparence (ils sont affichés avec un fond noir) !

- BUG : OBBox : Ces boîtes de collision ne fonctionnent actuellement pas !
- BUG : Maison créée (en construction) + création d'arbres en rectangles par-dessus (but : entourer la maison d'arbres) -> Les arbres sont aussi créés à l'intérieur de la maison !

- BUG : Construction Map : Lors de son agrandissement, certains pixels en diagonale ainsi qu'en bord de carte deviennent noirs, empêchant ainsi la construction (voir "Terrain.ewt" pour un aperçu du problème)

- BUG : Mode release : sortes "d'ombres vides" des bâtiments sur le terrain créant des trous dans lesquels on voit le ciel
	-> N'apparaît que lorsque l'anti-crénelage est désactivé !
	=> Apparaît aussi avec DirectX 8 (au moins en mode Debug fenêtré, mais avec un anti-crénelage de 2x)

- BUG : Problèmes de collisions avec le triangle selector de la caméra FPS (la caméra s'arrête toute seule au milieu du terrain, sans raison apparente, comme si la touche du clavier pour avancer avait été relachée) (-> Bug remarqué depuis l'implémentation des dalles en béton, mais ce bug apparaît même lorsqu'aucun bâtiment n'est créé)

- Problème : ToolTips : On voit parfois un tooltip d'un élément masqué lors du changement d'interface de jeu, et qui subsiste tant que la souris ne bouge pas (alors que ce tooltip n'était pas affiché lorsque son élément parent était visible)

- Problème : Collisions : Lorsque la caméra FPS est sur un bâtiment en construction et qu'il se termine, la caméra FPS reste bloquée dans le nouveau triangle selector

- Problème : Impossible de créer les bâtiments de la prévisualisation en rectangle en-dehors des limites terrain système : on voit seulement le bâtiment de prévisualisation à la position pointée par la souris
	-> A revérifier : peut-être déjà corrigé





En cours :

- Actuellement :
	=> Ombres de XEffects désactivées en mode Release (option du menu désactivée)
	=> Sélection des bâtiments buggée (problème interne à Irrlicht !)



A voir rapidement :

- Lorsque le menu de construction descends automatiquement, attendre une ou deux secondes avant de le descendre une fois qu'il est en haut et que la souris vient de le quitter

- Problème : La version du fichier "EcoWorld (Kid version).exe" n'est pas "0.0.1.1" comme attendu (vient du compilateur de ressources)

- Revoir les échelles de tous les bâtiments :
		=> Toutes les tailles sont actuellement bonnes !
	+ Augmenter la hauteur entre la caméra FPS et le sol ?
	+ Revoir la position de départ de la caméra RTS (à chaque départ de partie je la déplace, même pour les tests rapides : c'est peut-être qu'elle n'est pas bien placée)
	=> Eviter les ajustements logiciels de la taille (blenderObjectsScaling * 0.5f) : les effectuer dans Blender directement
	+ Mettre à jour la valeur CAMERA_NEAR_VALUE et CAMERA_FAR_VALUE (à réduire légèrement si possible)
	+ Revoir toutes les positions des textes au-dessus des bâtiments et des fumées pour vérifier qu'elles sont toujours cohérentes (en particulier pour les bâtiments redimensionnés sous Blender)
	+ Tourner les bâtiments suivant de 180° dans blender :
		- Pompe d'extraction d'eau (90° seulement) (voir la position de la pompe d'extraction d'eau dans le jeu pour déterminer quelle rotation est la meilleure : avec la roue en face de la caméra par défaut)
		- Usine de verre
		- Usine de tuiles
		- Usine de papier
		- Centrale nucléaire

- Redimensionné : 1 carreau = 5 mètres réels	=> Revoir toutes les valeurs dépendantes des surfaces (production de blé...)
	- Centrale à charbon : x2	-> Agrandir fumées
	- Hydrolienne : x2			-> Voir pour profondeur minimale de l'eau
	- Eolienne : x2
	- Maisons (habitations) : *2/3 (TODO)
	- Panneaux solaires

- BUG : Lors de la création d'une maison mesurant 6x3 unités :
	la maison est effectivement bien placée sur la grille en largeur (3 unités), mais elle chevauche des demi-unités sur la longueur (6 unités), elle serait bien placée si sa longueur était impaire
		=> Pouvoir placer correctement des bâtiments de dimension paire
		=> Ou : N'avoir que des bâtiments de dimension impaire ! (ajouter +1 à la taille du bâtiment si sa taille est impaire)

- BUG : Le grand building individuel n'est pas en face de son bloc de béton !
	+ Même problème pour les panneaux solaires

- Faire fonctionner les OBBox pour détecter les collisions lors du placement des bâtiments

- Instructions sur les couleurs des matériaux des bâtiments sous Blender (todo.txt)

- Créer les textures des Normal Map manquantes avec la nouvelle méthode (avec leurs qualités différentes) :
	=> Modifier la méthode pour prononcer l'effet de Normal Mapping (ajouter des itérations de normalisation grâce au plugin NormalMap (modifier la valeur Size à plus de 1) juste avant la Normal Map Finale)
		- Blocs de béton
		- Route
		- Panneaux solaires
		- Maisons
		- Chalet (à recréer complètement !)
		- Immeubles (à recréer complètement !)
		- Buildings (à recréer complètement !)

- Créer les dernières architectures 3D des bâtiments + Colorer ces bâtiments

- Pouvoir déplacer les bâtiments
- Améliorer les routes

- Réactiver les ombres stencil (seule la lumière de ces ombres semble manquer lorsque l'eau avec shaders est utilisée ; sans l'eau avec shaders le rendu est buggé : on ne voit que le ciel et le soleil)

- Eau avec shaders : Ajouter un brouillard au fond de l'eau pour donner une impression de profondeur (la lumière devrait être plus rapidement absorbée avant d'atteindre le fond)
	=> Sûrement modifiable dans les shaders directement (mais risque peut-être d'entrer en conflit avec le brouillard automatique de DirectX)

- Gisements + Pouvoir choisir un terrain avec des arbres directement dessus
- Vente/Achat de ressources

- Utilité d'une implémentation d'une destruction rectangulaire des bâtiments ?
	=> Bouton Destruction -> Tracé d'un rectangle à la souris -> Relache le bouton gauche de la souris -> Question "Voulez-vous supprimez ces XXX éléments ?"

- Réorganiser l'ordre de l'affichage des informations dans la fenêtre d'informations (au brouillon tout d'abord !) : actuellement non cohérent !

- La vue de la mini-carte n'est affectée ni par les effets post-rendu, ni par les ombres de XEffects

- Pour le bon fonctionnement de XEffects, il faut activer les lumières d'Irrlicht ! : Simplement passer la couleur de la lumière de XEffects à blanc pour ne pas atténuer 2 fois les couleurs
- Ajouter d'autres types d'ombres depuis des shaders
	-> Implémentation de XEffects dans Game (Voir ici pour plus d'informations sur le Shadow Mapping : http://en.wikipedia.org/wiki/Shadow_mapping ; et : http://takinginitiative.net/2011/05/15/directx10-tutorial-10-shadow-mapping/)
		+ Tester le bon fonctionnement de l'activation XEffects depuis le menu options
		+ Activer aussi cette option en mode Release + Etendre ses possibilités (qualité des ombres, lié à leur distance...)
		+ Dans EcoWorldRenderer::update : Gérer le cas où initSunPosition est à true : redéfinir la position réelle de la lumière de XEffects (voir EcoWorldRenderer.cpp : ligne 3890)
		+ Régler les paramêtres Near value, Far value et Fov (la zone carrée dans laquelle la lumière éclaire) dans la création de la lumière pour les ombres
		+ Créer une CLightCamera d'après les lumières des ombres pour permettre d'effectuer sceneManager->setActiveCamera(lightCam) avant le rendu de la profondeur dans la vue de la lumière
			-> Permet ainsi aux scene nodes qui sont rendus dans cette pass de s'optimiser suivant la vue actuelle
		+ Vérifier si on peut supprimer l'instruction shadowNode->OnRegisterSceneNode juste avant son rendu : en effet, cette instruction n'est pas censé l'animer, et ralenti le rendu en l'ajoutant au scene manager (et risque aussi de l'afficher plusieurs fois dans le rendu du scene manager !)
		+ Vérifier si le temps de rendu est diminué en changeant le statut des ombres du terrain de BOTH à RECEIVE (évite un rendu supplémentaire du terrain), et celui des nodes (des bâtiments) de BOTH à CAST
			-> Si le temps de rendu est effectivement diminué, ajouter des options dans GameConfiguration pour pouvoir régler leur mode de réception/envoi des ombres (de BOTH à NONE pour chacun d'eux)
		+ Permettre à l'utilisateur d'ajouter des effets post-rendu (à travers le menu options) + Ajouter d'autres effets post-rendus personnalisés en s'inspirant du SDK de DirectX 11 et des post-effets de Morrowind Graphic Enhancer (version GUI)
		+ Permettre (grâce à des defines du préprocesseur de shaders) de gérer les lumières directionnelles plus efficacement dans XEffects (pas de position de lumière (seulement une direction), pas de distance max, pas d'atténuation...)
		+ Vérifier que le rendu final des lumières dans XEffects et dans Irrlicht (de base) est identique !
		+ Shaders et callbacks de XEffects à optimiser (utiliser gl_XXX et envoyer les index des textures en OpenGL)
		+ Au lieu de compiler tous les shaders dès la création de XEffects, compiler les shaders nécessaires dès qu'un node prévoit de les utiliser (dès la création d'une lampe par exemple : compiler les shaders avec le filtre spécifié s'ils n'ont pas encore été compilés)
			=> Accélération du chargement de XEffects et du menu principal !
		+ Bug à résoudre entre XEffects et l'eau avec shaders
		+ Vérifier que les objets en-dehors de la shadow map sont bien rendus illuminés et non pas en ombre
			+ BUG : Le normal mapping ne fonctionne pas avec XEffects puisqu'aucune lampe n'est assignée au driver, de même pour les shaders de terrain Copland et N3xtD
				=> Il faut activer les lumières standards d'Irrlicht pendant la pass de rendu de la scène, et n'utiliser la shadow map que pour assombrir la scène !
		+ Etudier l'autre version de XEffects : OrthographicShadow (fonctionne avec les lumières directionnelles !)
		+ XEffects gère mal la prévisualisation des arbres (-> BUG lorsque l'arbre de prévisualisation est détruit) (peut-être parce qu'ils sont transparents ?)
		+ XEffects ne crée pas d'ombres sur les objets transparents
		+ Résoudre les problèmes de culling dans la vue de la lampe (appuyer NumPad 1 pour un aperçu de la depth map)
		+ Vérifier que les animations ne sont pas 2x plus rapides avec XEffects
		+ Le brouillard semble être activé même lors des rendus internes de XEffects (depth map et light map) sous DirectX
		+ Vérifier que XEffects ne désactive pas le brouillard (en OpenGL surtout)
		+ Améliorer la précision du depth buffer en utilisant : dist = tex.r + tex.g / 256.0f dans les shaders
		+ Vérifier que le déplacement dynamique de la lumière de XEffects fonctionne bien en mode caméra RTS et FPS
		+ Eviter l'appel à driver->getTexture (lent) pour obtenir les shadows maps (les stocker dans une core::map ?)
		+ Désactiver tous les matériaux et les textures des scene nodes affichés lors de la pass de rendu de la shadow map : optimisation du temps de rendu + désactiver les autres lumières...
		+ Animer les nodes avant les pass de rendu seulement lorsque nécessaire
		+ Interdire le support des lumières ponctuelles (optimisation), et pouvoir directement indiquer light->setDirection plutôt que light->setTarget (le premier étant le plus adapté pour ces lumières)
		+ Vérifier que la direction de la lumière utilisée est bien la bonne (indiquée dans RTSCamera::updateXEffects)
		+ Tester le résultat final en mode OpenGL et DirectX 9, et déterminer les valeurs minimales PS_xxx et VS_xxx nécessaires pour chaque shader (versions des PS et VS)
		+ Utiliser la valeur de la couleur des ombres des temps (WeatherInfos::shadowColor) pour déterminer la couleur des ombres de XEffects
	-> Vérifier que TOUS les terrains reçoivent bien les ombres, ainsi que l'eau et les objets 3D eux-mêmes, et aussi les objets transparents (transparence ALPHA_REF surtout : transparence ALPHA non officiellement supportée, mais à tester tout de même si possible)
	-> GUI menu options à remodifier (Créer des onglets dans le menu options + l'adapter aux nouvelles options ajoutées)

	=> A noter dans la liste des améliorations de version (Menu options amélioré + Ajout d'options dans la configuration du jeu + Ajout des ombres de textures et des effets post-rendu grâce à XEffects)



- Implémentation des effets post-rendu :
	- BUG : La première frame du jeu ne contient que le ciel (lorsque ses shaders sont activés) lorsque post process est activé, si les shaders du ciel sont désactivés, la première frame du jeu semble être d'une couleur aléatoire, proche de la couleur dominante du sol
	- Ajouter option calcul HDR (High Dynamic Range Lighting) + Autres effets divers de Morrowind (MGE et MGE XE)
		-> Voir ici pour de nombreuses informations sur les shaders : http://www.scribd.com/doc/72782053/73/Your-First-HDR-Shader-The-Glare
		(HDR : voir page 164)
	- Permettre l'envoi de données supplémentaires aux shaders :
		- HDR (voir ci-dessus)
		- sunVec, sunCol, eyeVec, eyePos, sunVis ? (voir sunshafts.hlsl), rcpres ? (voir sunshafts.hlsl), waterLevel, fogRange, fov
			-> Vérifier automatiquement si les données des HDR doivent être envoyés aux shaders à chaque application des paramêtres dans le menu options des effets post-rendus (Vérifier les données nécessaires pour chaque effet)
	- Diminuer tant que possible les versions des shaders utilisées par PostProcess (idéal, même si impossible : tous les vertex et pixel shaders utilisent les versions VS_1_1 et PS_1_1)
		+ le shader de Depth inclus (pour la depth pass)
	- Régler le format et la précision de la texture RTT rttDepth : il est actuellement inutile qu'elle soit codée sur 32 bits RGBA
		alors qu'elle n'utilise que des nuances de gris dans la composante alpha (en effet les donnéees des normales stockées dans les composantes RGB de la texture ne sont jamais utilisées) !

- BUG : Mode multijoueur : Lors de la construction de bâtiments, la date de construction est rarement synchronisée ! (lorsque le message prend plus d'un jour à être envoyé par exemple) (=> visible car le temps de vie des bâtiments n'est pas synchronisé)
	=> Envoyer le jour du sytème de jeu lors de l'envoi de chaque message pour que les clients sachent lorsque un message a été activé, et puissent se synchroniser
- Version multijoueurs en réseau :
	+ Permettre à l'utilisateur jouant en mode serveur d'envoyer les informations sur sa partie à un client (en utilisant l'ID ID_ADVERTISE_SYSTEM pour lui indiquer notre adresse et port) :
		/// RakPeer - Inform a remote system of our IP/Port. On the recipient, all data past ID_ADVERTISE_SYSTEM is whatever was passed to the data parameter
		ID_ADVERTISE_SYSTEM,
	+ GUI multijoueur à modifier pour permettre les parties sur Internet (adresse IP et port pouvant être indiqués par l'utilisateur)
	+ Permettre à l'utilisateur de jouer en ligne une partie sauvegardée (qu'elle ait été jouée en ligne précédemment ou non)
	+ Spécialiser la construction de bâtiments en rectangle, plutôt que d'utiliser plusieurs fois de suite la construction simple de bâtiments (qui est très coûteux en bande passante)
	+ Gérer la déconnexion des clients au serveur (bloque le jeu dans ce cas !) et inversement (notification), ainsi que leur connexion
		-> Créer un tableau affichant tous les clients multijoueurs et leur ping, ainsi que des boutons pour les éjecter de la partie (en permettant la simple déconnexion ou le banissement définitif (feature de RakNet))
	+ Organiser le PacketLogger actuel d'EcoWorld sous forme de tableau
	+ Utiliser le StringCompressor de RakNet pour l'envoi des parties sauvegardées (voir StringCompressor.h et StringCompressor.cpp dans la source de RakNet)
		=> Impossible car "It only works with ASCII strings"

- Créer le bâtiment de la scierie (sous Blender)
- Voir la TODO list des bâtiments sous Blender (dans le dossier Textures)

- Utiliser les outils de Irrlicht i18n (outils de traduction, de font vectoriel...)



A faire (relativement rapide et/ou simple) :

- Optimiser l'utilisation de la mémoire de la version enfant (supprimer les variables inutiles de la GUI, éviter les trops grandes allocations de mémoire pour les ressources dans les informations statiques des bâtiments, ...)

- Ajouter un type d'objectif au jeu : quantité de ressource : la quantité de ressources du joueur du type ressID doit vérifier : ressID 'comparisonOp' dataF pour que cette condition soit vérifiée

- Tester intensivement le nouveau système de mise à jour du monde et des bâtiments dans des parties réelles !

- Changer la couleur du fond de l'eau en fonction du temps : claire lorsque soleil, grise lorsque pluie...
	+ Avec l'eau avec shaders : augmenter le brouillard lors du rendu de la partie sous l'eau par rapport au brouillard en-dehors de l'eau pour faire l'effet de l'obscurité naturelle de l'eau

- Réactiver le mode 3D pour le son de la destruction du bâtiment terminée

- Diminuer la densité des sons de l'eau (flagrant sur le terrain Highlands old !)
	-> Provient sûrement du fait que les échelles dans Irrlicht ne sont pas les mêmes que sous irrKlang (1m != 1.0f !)
	+ Prendre en compte les positions de terrain en-dehors de la carte système pour la détermination du placement des sons de l'eau

- Mettre des couleurs réalistes sur les usines (toit rouge, murs gris...)

- Tester l'utilisation du logo spatial à Romain DARCEL en tant que fond du menu principal
- Ajouter options fond du menu principal : soit : - Fond de chargement ; - Terrain plat (tourne autour) ; - Ville complète (animation)

- Diminuer les rendus pendant que le jeu est en pause pour une même position de la caméra en utilisant les RTT pour mettre le fond du jeu en cache, puis en affichant cette RTT grâce au screen quad à chaque rendu
	=> Seulement lorsque le menu Echap est affiché ! (trop de conditions à vérifier autrement : bâtiment séléctionné, bâtiment pointé, bâtiment à construire, position et rotation de la caméra...)
		=> Abandonné : ne vaut pas le coup !

- Créer un fondu entre les titres "EcoWorld" du menu principal pendant les changements de temps du menu

- Dans le texte d'informations, afficher les ressources manquantes pour construire le bâtiment sélectionné en rouge

- Faire que la fumée qui sort des bâtiments soit proportionnelle au facteur de production actuel de ce bâtiment
	+ Ne pas directement "masquer" la fumée des bâtiments dès que leur facteur de production s'annule : faire que les dernières particules de fumées continuent à monter
		=> Incompatible avec le mode de gestion actuel de la fumée : un seul système de particules pour plusieurs cheminées

- Créer une option dans les terrains pour qu'EcoWorld n'utilise pas la Construction Map fournie, mais qu'il la calcule automatiquement d'après les hauteurs du terrain (non constructible lorsque : terrain sous l'eau ou assez proche OU fort dénivelé ; constructible sinon)
	(comme il calcule déjà si de l'eau profonde est disponble ou non)

- Implémenter la réfraction de l'eau (eau avec shaders) (les objets dans l'eau ne doivent pas être alignés avec les objets hors de l'eau, dû à la réfraction de la lumière dans l'eau)

- Dans GameConfig et le menu options, et en utilisant le préprocesseur de shaders de XEffects : permettre d'activer ou désactiver la réflexion de l'environnement sur l'eau avec shaders (car cela nécessite un rendu supplémentaire à chaque frame, pour un effet relativement peu visible)

- Recompiler toutes les librairies et implémenter les modes de compatibilité et les modes 64 bits (créer des options spécifiques dans chaque projet)
	+ Vérifier les nouvelles versions de toutes les librairies utilisées

- Ajouter une option pour effectuer le rendu en 3D (filtres bleu/rouge) : voir http://irrlicht.sourceforge.net/forum/viewtopic.php?t=33463

- Mettre à jour complètement la liste des parties multijoueurs régulièrement (toutes les secondes par exemple), pour éviter que des parties quittées par le serveur ne restent encore dans la liste

- N'activer la construction réctangulaire qu'après 200 à 300 ms d'appui sur le bouton gauche de la souris, pour permettre par exemple de créer un arbre simple rapidement
	+ Activer la construction rectangulaire pour tous les bâtiments ?

- Créer une option dans GameConfiguration pour remplacer le define USE_VARIOUS_TREES + Créer une case à cocher dans le menu options pour ce paramêtre

- Revoir la pluie de Spark pour qu'on ne puisse pas voir le départ des gouttes d'eau (elles tombent d'abord verticalement, puis se redressent peu à peu suivant une ligne oblique)
	-> Faire qu'elles tombent suivant une droite dès leur départ jusqu'à leur arrivée, et non pas en traçant une parabole "sous l'action du vent"

- Créer plusieurs niveaux de zoom dans la mini carte

- Créer un bouton dans la GUI du jeu pour mettre le jeu en pause
- Créer un bouton dans la GUI (dans le menu de droite) pour permettre d'afficher le menu d'Animation de la caméra

- Lorsque RakNet ne peut être créé (dans le menu multijoueur ou lors de la création d'une partie multijoueur), afficher un message à l'utilisateur (dans le jeu, pas dans le log) indiquant clairement pourquoi RakNet n'a pu démarrer (d'après le code d'erreur qu'il a retourné lors de sa création)
	+ A faire aussi lorsque c'est la connexion à un hôte (ou d'un client au serveur (joueur actuel)) qui a échouée

- Utiliser un tribool pour Game::vitesseMenuInferieur à la place d'un float actuellement

- Rétablir la transparence de la barre supérieure de la GUI du jeu

- Faire des tests sur les shaders lorsqu'ils ne sont pas supportés (possibilité de simuler une feature non supportée par le driver : désactiver PS_2_0 ou VS_2_0 et supérieur, et vérifier que le jeu ne plante pas)

- Ajouter les lumières spéculaires au shader de l'eau actuel (donne un effet magnifique avec les reflets du soleil !)
	+ Permettre le rendu de l'eau avec shaders sans réflection/réfraction et sans lumière spéculaire (suivant des options dans GameConfiguration), en utilisant le shader preprocessor de XEffects dans les shaders de l'eau

- Créer des options dans GameConfig pour gérer :
	- La qualité des systèmes de particules (multiplicateur du nombre de particules émises), plus une option pour les désactiver complètement (pas d'appel à SparkManager::init() dans ce cas)
	- La qualité des arbres (multiplicateur de chacun des paramêtres des niveaux de détail (highLOD et midLOD gérés séparéments !) + multiplicateur du nombre de feuilles)
		-> Bug avec les arbres : le début du tronc n'est pas en coïncidence avec le reste de l'arbre (n'apparaît qu'avec des niveaux de détail bas)
		-> Pouvoir diminuer le nombre de feuilles des arbres
	- La qualité des terrains (multiplicateur (x2, x4, x0.5, x0.25) de la taille de la height map utilisée pour créer le terrain)
	- Le format d'enregistrement des captures d'écran (bmp, jpg, png...)

- Transformer le tableau des ressources IGUITable ? (avec des boutons en face de chaque ligne pour l'achat/la vente de la ressource)

- Gérer les cas dans Game::render où driver->beginScene et driver->endScene échouent (ils retournent alors false)

- Revoir la position de tous les textes au-dessus des bâtiments

- Faire que la touche "Back space" ait le même effet que la touche Echap dans les menus

- Améliorer la fumée 2 (-> de la centrale à charbon et de l'usine d'incinération des déchets)
- Utiliser des systèmes de particules pour la création/destruction d'un bâtiment
- Revoir le modèle de particules de la pluie

- BUG : A leur placement, les bâtiments (bien qu'étant à la position exacte de leur prévisualisation) ont souvent une partie de leur zone de construction "sous le sol",
		malgré la sécurité prise par EcoWorldRenderer::getNewBatimentPos de calculer la hauteur maximale du terrain sur la surface du bâtiment
	-> BUG Corollaire : Certaines routes se retrouvent actuellement complètement en-dessous du sol (voir partie "Thomas (Grande).ewg")

- BUG : On ne voit parfois pas tout le texte de la description des objectifs dans la liste modifiable des objectifs : Trouver un moyen de le faire défiler
	-> L'englober dans un élément parent CGUIScrollListBox contenant une IGUIListBox et une IGUIScrollBar horizontale pour la faire défiler horizontalement

- BUG : Quelques petits bugs de passage à la ligne des textes sous certaines résolutions => Utiliser les font vectorielles !
	-> Exemple : En 1024x768 : Informations de la satisfaction ("Satisfaction : 100 %\r\nSatisfaction réelle : 100 %") : Passage à la ligne au dernier "%" !



Modifications à apporter à Irrlicht :

- BUG : Les accents dans les noms de fichiers ne sont pas supportés dans la liste de la fenêtre d'enregistrement/chargement de partie,
		mais ils sont supportés dans son edit box du nom de fichier (ne vient donc pas de la police utilisée !)
	=> Vient du système de fichier d'Irrlicht qui ne supporte pas les caractères spécifiques wchar_t dans les noms de fichier
		-> Peut être résolu en passant le système de caractères d'Irrlicht pour les noms de fichiers en wchar_t



Important (jouabilité et possibilités de jeu pour le joueur) :

- Insérer les dernières options de GameConfig dans la GUI du menu options : voir CGUIOptionsMenu.h

- Créer plusieurs curseurs pour le jeu (Irrlicht peut gérer les curseurs système : voir ICursorControl.h)

- Dans le menu d'animation de la caméra RTS, permettre au joueur d'insérer des points où il veut, et de déplacer les points existants dans la liste (monter ou descendre le point sélectionné)

- Dans le module d'animation de la caméra RTS, permettre aussi de faire suivre un chemin personnalisé au point cible (target) de la caméra
	-> Créer un animator personnalisé pour gérer une caméra et déplacer sa cible en même temps que sa position

- Permettre au joueur de régler précisément le budget et les ressources de départ pour une partie

- Ajouter des bruits de fond d'environnement (oiseaux, plus de vent (variété des sons)...)
	+ Les sons manquants dans l'IrrKlangManager

- Implémenter la destruction rectangulaire des bâtiments (dans BatimentSelector et aussi dans RakNetManager)

- Créer une file de messages de jeu à gauche de l'écran (style Anno 1404) permettant d'informer le joueur de chaque action importante (destruction d'un bâtiment, manque important d'énergie...)

- Implémenter la vente/l'achat de ressources
	-> Nécessite la valeur réelle de chaque ressource

- Créer le tableau récapitulatif des fins de mois tel que prévu sur papier, et renommer le tableau actuel en "Liste détaillée des bâtiments"
- Implémenter des graphiques : budget/ES/déchets/habitants... en fonction des jours/mois/années

- Implémenter les derniers bâtiments

- Objectifs : Informer le joueur lors d'un objectif réussi/perdu
	+ Implémenter les objectifs facultatifs

- Menu partie gagné : Afficher un tableau des objectifs remportés/perdus (+ leur date de dernière réalisation)

- Débloquer les bâtiments au fur et à mesure dépendant de la population actuelle
	=> Lorsque la population retombe en-dessous du seuil minimal pour ce bâtiment : rebloquer le bâtiment ?

- Implémenter les scénarios (jeu déjà créé + terrain associé éventuellement) ; Archive zip sous extension "ews" contenant un fichier "Scenario.xml" avec les informations sur le scénario
	-> Difficulté réglable grâce aux modifiers (modifiers personnalisés dans les scénarios), mais aussi possible grâce au chargement de différentes parties (plus ou moins faciles) suivant la difficulté
	-> La difficulté peut aussi être forcée à certaines valeurs (ex : seulement Normal ; Normal ou Difficile...)
	-> Image de prévisualisation du scénario + Texte de description

- De même, implémenter les campagnes (Archive zip sous extension "ewc") :
	ensemble de scénarios ayant un ordre et (en option réglable dans la campagne -> Peut aussi être désactivé) un niveau de difficulté

- Revoir les routes : ne plus les utiliser en tant qu'objets 3D mais en tant que texture rajoutée par dessus la texture de couleur du terrain (-> Peut-être utiliser un shader)
	-> Utiliser la même méthode pour dessiner la grille du terrain

- Pouvoir acheter des terrains (nombre limité et fixé : 36 terrains achetables)
	-> Support dans EcoWorldSystem ; Menu achat de terrain ; Adaptation de la mini-carte par rapport aux terrains achetés
- Implémenter les gisements (fichier dans le terrain "Gisements.xml" indiquant leur type et leur position) en 3D et dans le système de jeu

- Tas de gravat créés à la place d'un bâtiment détruit par fin de vie

- Créer un fichier 3D avec des bouées pour les zones de construction sur l'eau

- Réorganiser le menu de construction pour qu'il prenne moins de place sur l'écran
	+ Le réorganiser en plusieurs sous-écrans accessibles par des boutons fléchés
	+ Simplifier grandement l'ajout de nouveaux boutons grâce à une grille prédéfinie

- Créer un bouton pour afficher/masquer la grille du terrain
- Permettre de réduire la barre de réglage de la vitesse du jeu

- Avancé : Créer une console (style Morrowind) pour avoir le contrôle complet sur le système de jeu



Important (général) :

- Lors de la construction de bâtiments :
	si le bâtiment à construire ne peut être créé, faire disparaître progressivement le(s) bâtiment(s) de prévisualisation utilisé pour sa prévisualisation (avec une transparence alpha progressive : dans le shader de NormalMapping),
	sinon, s'il peut être créé, faire apparaître un nuage de poussière (grâce à Spark) à l'endroit où il a été implanté (comme dans Zoo Tycoon 2 quand les bâtiments apparaîssent instantanément)				

- Utiliser un mesh combiner pour améliorer les performances du jeu lorsque de nombreux nodes ont été créés (peut par exemple être utilisé pour combiner plusieurs arbres ou routes entre eux, accélérant ainsi leur calcul)

- Permettre l'accès au menu options pendant le jeu

- Créer des paramêtres pour pouvoir paramêtres les touches de contrôle de la caméra et les touches de raccourcis

- Créer une nouvelle eau grâce à des shaders provenant de XEffects (voir Exemple4)

- Créer un header dans les sauvegardes ECW pour, lors du chargement des parties sauvegardées, pouvoir réellement déterminer si ce sont bien des sauvegardes EcoWorld

- Créer une classe GameLoader (pour la création des nouvelles parties, le chargement de parties sauvegardées, la gestion des versions des sauvegardes (!), de scénarios, de campagnes...)
	Ceci permettrait de décharger la classe Game de plusieurs centaines de lignes de code
- Créer une classe CGUIUpBar gèrant la barre supérieure de la GUI du jeu, CGUIRightBar gèrant la barre de droite de la GUI du jeu et permettant de masquer la barre de commande de la vitesse du jeu, CGUIConstructionMenu gèrant le menu inférieur de construction pour décharger Game de la gestion de ces entités
- Créer une classe pour chaque type de GUI du jeu : CGUIMainMenu, CGUIOptionsMenu, CGUINewGameMenu, CGUIGameInterface, CGUILoadingScreen (déjà créée)

- Faire que les fonctions de création d'une nouvelle partie retournent un bool, et retournent true et affichent un message d'erreur lorsqu'une erreur survient lors du chargement du jeu
	-> Faire de même pour les sous-fonctions importantes de celles-ci (ex : EcoWorldTerrainManager::loadNewTerrain)

- Rendre les infos sur la position des fenêtres de la GUI indépendantes de la résolution choisie dans les options !

- Revoir les sens des bâtiments créés dans Blender et à leur création dans EcoWorld, ainsi que leur taille finale, leur agrandissement et leur position (certains ne sont pas centrés)
	-> Utiliser TAILLE_OBJETS = 1.0f !
	-> Taille des maisons + usines à multiplier par 2
		-> Revoir la taille relative des éoliennes et des hydroliennes
	-> TAILLE_CARTE vaut 6000 pour un terrain de départ de 2km + 4 terrains achetables de 1km
		-> Diviser les cartes en plusieurs sous-parties
	-> Régler aussi les informations de la caméra d'après ces nouvelles valeurs, et surtout le Target.Y (il est dû au bug qui semble faire tourner la caméra autour de sa position dans des terrains où la distance en hauteur entre son target et le terrain est très grande)

- Dans le tableau récapitulatif : créer une ligne de Total
- Dans le tableau récapitulatif : ne pas replacer la scroll bar en haut à chacune de ses mises à jour, et conserver sélectionnée la ligne actuelle
	-> Eviter d'effacer puis de recréer toutes les lignes du tableau à chaque mise à jour (de plus : le système actuel est très long !)

- Pouvoir enregistrer les miniatures des jeux avec leur fichier d'enregistrement (une capture d'écran qui serait visible lorsqu'on sélectionnerait le jeu dans la liste des fichiers)

- Utiliser la CGridSceneNode pour l'affichage de la grille du terrain
- Permettre en appuyant sur une touche du clavier de voir le terrain constructible/non constructible/en eau profonde (seulement possible en mode DEBUG actuellement -> rendre l'affichage de la grille plus compréhensible, ainsi que plus optimisé)
- Pouvoir voir les limites du terrain et le terrain achetable
	-> Pouvoir acheter du terrain

- (Ne pas créer toutes les GUI d'un coup lors de l'appel au GUI Manager : créer les GUI du jeu (Jeu et Jeu perdu) seulement lorsque nécessaire (pendant le chargement principal du jeu))

- Rendre SPARK indépendant du device (comme IrrKlang : pouvoir changer de driver sans le remettre complètement à zéro)

- Optimiser la classe obbox2df

- Créer une caméra pour une vue en mode "carte postale" comme dans Anno 1404



A faire (qualité professionnelle et complète du jeu) :

- Créer d'autres terrains (les dessiner avant sur papier)
	-> Créer un terrain sablonneux (majoritairement), un terrain aux couleurs tropicales (couleur émeraude majoritairement), un terrain dans les neiges, un terrain sur un volcan
- Créer d'autres fonds d'écran de chargement, plus récents et professionnels
	-> Ils devraient avoir une résolution de 1920x1080 (Full HD)

- Utiliser un outil de localisation (traduction) vers l'anglais et/ou d'autres langues pour traduire les chaînes de caractères en langue françaises du code
	-> Utiliser GNU gettext si possible

- Permettre de modifier les touches par défaut du jeu (implémentation grâce à un tableau touches -> actions (ACTION touches[KEY_COUNT];) + Fonction OnEventAction(ACTION act) gérant les différentes actions possibles)

- Vérifier tous les fichiers externes utilisés, ainsi que tous les morceaux de code empruntés
- Créer un "copyright" ainsi qu'une license pour le jeu (pour autoriser sa diffusion : prévue vers juin 2012)
	-> Prévoir une version payante du jeu (quelques euros ? (prix vus sur le forum d'Irrlicht : un jeu vaut environ 10~15€ : il pourrait facilement être vendu entre 2 et 5 €)

- Créer un système de mise à jour automatique (avec le module auto-patch de RakNet)

- Créer une vidéo d'introduction avant d'arriver au menu principal (vidéo de démonstration + faisant office de "crédits")
	-> Utiliser Bink (RAD Video Tools) (avec une version plus ancienne mais sans pub ? => Non car illégal : empêcherait la diffusion du jeu)

- Animer le fond du menu principal (avec la vidéo de fond)

- Créer un setup/launcher pour installer le jeu, DirectX 9 (DirectX 8 étant déprécié), et gérer son lancement automatique depuis un CD-Rom (affichage d'un menu et détéction de l'installation actuelle)
	-> A faire

- Créer un éditeur de terrain et de parties sauvegardées, scénarios, campagnes
	-> Lorsque le projet sera terminé (A faire pendant les vacances d'été 2012 ?)

- Quitter la librairie d'Irrklang ? (relativement limitante, en termes de droits, d'options (non open source)...)



A faire (divers) :

- En mode multijoueur, permettre au serveur de n'accepter des clients qu'en mode observateur (ils ne peuvent produire aucune action, seulement regarder la ville)

- Essayer d'utiliser les joysticks pour contrôler le curseur de la souris
- Permettre au joueur de choisir le joystick à utiliser (par une liste des joysticks détectés dans le menu options)

- Modifier les curseurs affichés suivant la commande actuellement sélectionnée (normal, construction, destruction, bâtiment pointé, rotation de la caméra...) (possible par l'envoi directement dans EcoWorld de messages à la fenêtre Windows affichée)
	-> Créer un CursorManager pour gérer tous les curseurs du jeu, et supprimer les curseurs des ressources du jeu
	+ Utiliser des curseurs animés (voir http://www.codeguru.com/forum/showthread.php?t=444335)

- Mieux gérer les exceptions C++ qui pourraient arriver : voir http://www.codeproject.com/KB/exception/excv2.aspx

- Feature de replay ? (comme dans The Sleepless : enregistrer le temps système quand chaque action a été effectuée et les paramêtres de l'action)
	=> Dans ce cas, éviter les documents XML : très lourds ; préférer de simples documents textes, avec une action par ligne

- Faire que la caméra RTS ne puisse pas traverser les bâtiments

- Tester une version portable (sur Android/IPhone)

- Créer une fenêtre indiquant la consommation des ressources des habitants et l'influence de chaque ressource sur leur satisfaction

- Trouver un meilleur moyen d'animer les arbres que de les agrandir/rétrécir linéairement

- Dans la fenêtre d'informations sur le bâtiment sélectionné : indiquer le loyer/facture d'électricité/facture d'eau maximal possible (ex : "Facture d'eau (M) : 500 € / 2000 €")

- Destruction (voire sélection ?) de bâtiments en rectangle
- Sélections multiples ?

- Tableau récapitulatif des fins d'années/de mois : créer un affichage possible pour les mois précédents
- Remplacer le texte d'informations par un tableau (avec possibilité de mise en forme) : un IGUITable

- Il y a quelques bugs dans le placement des batiments : près des arbres (ou près du bord du terrain ?), une erreur "place insuffisante" est lancée alors que la place est libre
- Dans la caméra RTS, zoomer en utilisant le FOV de la caméra, et non pas en rapprochant/éloignant la caméra

- On voit peu le titre lorsque le temps du menu principal est à 3 (WI_raining) (ce titre-ci est peu visible avec tous les temps : Illuminé Jour)
- Petites coupures verticales dans le titre du menu principal à supprimer

- Les hydroliennes fixées au sol doivent toucher le fond !

- Pouvoir indiquer le type de brouillard dans WeatherInfos ?

- Permettre que le règlage dans les options du jeu des différents niveaux de qualité de texture soit appliquable aussi dans les terrains

- Ne pas pouvoir modifier la rotation des hydroliennes/éoliennes pour les conserver face aux courants marins/au vent (-> qui ont tous les deux la même direction)




- Rechercher une meilleur caméra FPS (la caméra FPS d'Irrlicht est décrite pour les tests, comprendre pourquoi (lente, peu précise, buggée... ?))

- Vérifier le comportement du jeu avec des terrains plus petits que la taille de la carte système
- L'erreur "Hors des limites du terrain" doit-il aussi prendre en compte le terrain 3D ?

- Protéger la construction en zone rectangulaire lorsqu'une zone est trop grande (la consommation mémoire devient alors énorme !)

- Le triangle selector du terrain Paged ne fonctionne pas !!!
	-> Gros bugs dans le terrain Paged, le rendant injouable !

- Pouvoir indiquer la taille du monde système (TAILLE_CARTE et TAILLE_OBJETS) dans les informations du terrain (et les protéger avec un maximum et minimum !)
- Pouvoir indiquer les informations de la caméra dans les informations du terrain (sa position de départ notamment)
- Créer une nature map dans les terrains (ou l'incorporer à la construction map) indiquant les gisements et la nature de départ (surtout les forêts), ainsi que les bâtiments de départ
- Lors de la création des terrains, ne créer le triangle selector que pour les triangles qui sont à l'intérieur de l'espace du monde

- Bug de la loading bar en OpenGL
- Bug "Unsupported texture format" en OpenGL avec shaders de l'eau
- En OpenGL, la texture du ciel n'est pas bien placée sur le skydome

- Utiliser Boost pour écrire dans les fichiers XML
- Utiliser Boost pour supprimer les fichiers (au lieu de "remove")

- Rapprocher la couleur du brouillard de la couleur du ciel dans la gestion des temps (Weathers.h)

- Créer des outils pour la modification de certaines options du jeu : un EcwTerrainCreator pour créer/modifier les terrains, et un EcwConfigEditor pour modifier certains paramêtres avancés de "Game Configuration.xml"

- Gros problème dans l'ombre du grand building individuel !

- Indiquer les nouvelles tailles des bâtiments
- Normaliser les tailles des batiments pour que 1.0f <-> 1m réel
- Eviter les tailles de bâtiment impaires ?

- Commencer l'implémentation des bâtiments finaux

- Les ombres stencil des éoliennes ne sont dépendantes que de la rotation de l'éolienne (et pas de la position de la lumière des ombres)
	-> Dû à un problème de normales des éoliennes
- Les ombres stencil de l'hélice des éoliennes ne sont pas affichées
- Les normales des éoliennes sont tournées de 90° en X (afficher les données de débogage sur les éoliennes pour plus de précisions)
- Les animations des éoliennes comportent des coupures vers la fin de leur animation (ces coupures ne sont pas dues au changement de vitesse de leur animation)

- Pouvoir détruire les batiments par glissement ou par rectangle de sélection

- Diminuer les répétitions de code dans les méthodes placerBatiment

- Créer un nouveau timer grâce à Boost pour gérer seulement le système de jeu, et ainsi accélérer le jeu sans accélérer aussi les animations d'Irrlicht (scene nodes, animators et caméra)
- Utiliser Boost pour créer des threads différents (1 pour les calculs du système de jeu et des particules, et 1 pour le rendu Irrlicht)

- Les info-bulles de la GUI ne fonctionnent pas si le timer d'Irrlicht est arrêté (lorsqu'on est en pause par exemple)

- Ajouter d'autres sons

- Faire que quand on est en pause, on puisse encore bouger la camera
	-> Rendre la caméra RTS indépendante du timer d'Irrlicht

- Remettre le CGUIFileSelector au propre
- Remettre la caméra RTS au propre (supprimer toutes les variables inutiles, optimiser les fonctions...)
- Revoir les pourcentages de chargement

- Le bouton pour la maison en bois n'est pas bien placé dans la GUI (il est dans l'onglet des centres)

- La lumière pour les ombres stencil n'est pas vraiment à l'infini, résoudre ce problème avec une lumière directionelle (impossible actuellement, attendre une prochaine version d'irrlicht permettant les ombres avec des lumières directionnelles)

- Il reste encore quelques bugs mineurs entre la transition entre la caméra FPS et la caméra RTS

- Créer un bouton de test du volume du son sous chaque Scroll Bar pour régler le son

- Idée pour le fond du menu du jeu : on se balade dans une grande ville écologique qui se construit sous nos yeux

- Chercher des musiques et des sons correspondants plus au type de jeu et à l'époque actuelle

- Le LOD ne fonctionne pour le moment que pour la maison de test : l'implémenter complètement

- On ne peut pas enregistrer des types "long" dans les fichiers, essayer de résoudre ceci avec des pointeurs
- Dans GameConfiguration::save() et GameConfiguration::load(), utiliser des enum à la place des int quand c'est possible

- Utiliser des polices qui varient suivant la résolution (peut-être attendre les polices vectorielles)

- Réorganiser l'ordre des variables et fonctions dans Game.h
*/

/*
Idées Améliorations Thomas :
- Gisements
- Terrains de départ : ajouter des arbres

- Débloquer les bâtiments au fur à mesure (par rapport à la population actuelle)
	-> Bâtiments différents produisant la même ressources mais avec une rentabilité différente (ex : cabane de pêcheurs -> port de pêche -> pisciculture)

- Augmenter la zone non constructible des terrains actuels près des rivières + augmenter la taille des terrains

- Empêcher la caméra FPS de tomber en-dessous d'une certaine valeur + Indiquer un zoom maximum réel pour la transition entre FPS -> RTS

- Bâtiment détruit par fin de vie : tas de gravat : durée de vie infinie, déchets D : déchets D du bâtiment qui s'est détruit, coût en ressources D identique à celui du bâtiment détruit

- Fond du menu principal : chargement d'une partie sauvegardée (malgré un + long tps de chargement) : lecture de l'animation de cette partie
*/

/*
Liens utiles :
- Description complète du procussus d'éclairage 3D temps réel et shaders associés : http://pisa.ucsd.edu/cse125/2006/Papers/Lighting-Per_Pixel_Shading_and_Optimizations.pdf
- Calcul du brouillard en GLSL : http://www.ozone3d.net/tutorials/glsl_fog/index.php
- Description des shaders GLSL : http://www.irit.fr/~Mathias.Paulin/IUP-SI/M2/4-glsl.pdf
- Description des shaders en HLSL : http://mathinfo.univ-reims.fr/image/dxShader/cours/08-HLSL.pdf
- Fichier de ressources détaillé pour les propriétés de l'application : http://msdn.microsoft.com/en-us/library/aa381058%28VS.85%29.aspx
- Caractères pour les noms de fichiers interdits dans les systèmes d'exploitation : http://en.wikipedia.org/wiki/Filename et http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247.aspx
- Librairies automatiquement ajoutées par le compilateur de Visual C++ et conflits possibles entre-elles : http://msdn.microsoft.com/en-us/library/aa267384.aspx
- Chargement différé des DLLs : http://msdn.microsoft.com/fr-fr/library/151kt790.aspx ; http://www.codeproject.com/KB/DLL/Delay_Loading_Dll.aspx ; http://www.pcreview.co.uk/forums/delayload-pragma-fixed-whidbey-t1430386.html ; et : http://msdn.microsoft.com/en-us/library/7f0aews7.aspx
- Exceptions SEH de MSVC : http://members.gamedev.net/sicrane/articles/exception.html
- Conseils d'optimisation pour DirectX : http://msdn.microsoft.com/en-us/library/windows/desktop/bb147263.aspx
- Optimisation avancés des shaders HLSL : http://developer.amd.com/media/gpu_assets/Dark_Secrets_of_shader_Dev-Mojo.pdf
*/

/*
Nouveautés (Pour le ??/??/12 : Version 0.0.0.1)
- Ajout de l'option du facteur de visibilité des temps (weatherVisibilityFactor) dans la configuration du jeu, pour permettre une meilleure visibilité du jeu lors des temps sombres
- Réorganisation complète du menu inférieur de construction de la GUI du jeu et ajout du mode réactif à la descente/montée de ce menu : ce menu s'affiche lorsque la souris est sur lui, puis se masque lentement lorsque la souris quitte ce menu
- Nombreuses modifications et améliorations de la version enfant du jeu :
	- Réintégration des usines : elles ne consomment simplement plus les ressources non productibles par le joueur, mais continuent de produire leur ressource dédiée : permet la réintégration des ressources verre, tuiles et ciment.
	- Réintégration du papier : c'est maintenant une ressources consommée en grande quantité par les habitants pour leur satisfaction, ce qui ajoute un peu de difficulté à cette version du jeu et un intérêt à la satisfaction des habitants.
	- Pour la production quotidienne de bois des arbres : Les arbres produisent maintenant du bois proportionnellement à leur âge (jusqu'à 1 an : 5 kg/arbre/J).



Pour le 27/08/12 : Version 0.0.0.1 : Aucune rétrocompatibilité avec les sauvegardes créées dans des les versions antérieures du jeu !
- Création d'une version "Enfant" du jeu (Kid version) : Suppression complète des ressources non productibles : Cette version est dorénavant complètement jouable (bien que très simplifiée) !
- Ajout des dernières textures de Normal Mapping
- Suppression du type de terrain Paged car buggé et non adapté à ce type de jeu
- Ajout des effets post-rendu et des paramêtres correspondants dans le menu options. Ces effets sont complètement fonctionnels, seul problème mineur : l'anticrénelage doit être désactivé pour pouvoir les utiliser, sinon cela provoque des incohérences de Z-Buffer.
- Mode multijoueur réactivé : fonctionnel mais non stable sur de longues parties, dû à des problèmes de synchronisation
- Nouvelles règles : Evite que le joueur ne gagne de l'argent à n'avoir que des maisons sans leur fournir aucun confort (ni électricité, ni eau, ni aucune ressource pour leur satisfaction)
	- Les impôts sont maintenant dépendants du nombre de personnes dans les habitations
	- Les vitesses d'emménagement et de déménagement des habitants sont maintenant aussi dépendantes de leur satisfaction réelle (plus ils sont satisfaits, plus ils emménagent vite, et inversement pour leur déménagement).
		En-dessous de 20% de satisfaction réelle, les habitants déménagent des maisons. Le seuil de satisfaction (20% par défaut) est dépendant de la difficulté du jeu (de 10% en mode Facile à 40% en mode Difficile actuellement).
	- Chaque habitation a un minimum de 1 habitant, pour éviter que la population du monde atteigne 0, et donc pour pallier au problème suivant :
		Sans cette limite, lorsque la satisfaction de la population est faible, la population du monde diminue progressivement jusqu'à atteindre 0 habitants.
		Une fois que la population du monde est de 0 habitants, la satisfaction de la population passe instantanément à 100 %.
		La population du monde se remet alors à augmenter à sa vitesse maximale. Une fois que la population du monde n'est plus nulle, la satisfaction de la population rechute à sa faible valeur,
		et la population du monde diminue encore jusqu'à 0 habitants, et ainsi de suite. Ainsi, à des taux faibles de satisfaction il se crée une oscillation de la population entre 0 habitants et 1 habitant,
		et la satisfaction fait des sauts brusque de sa faible valeur à 100 %, ce qui rend la population du monde instable.
	- Le pourcentage d'énergie disponible pour les habitations influe aussi sur le pourcentage de satisfaction des habitants (facteur entre *0.5 à 0% d'énergie à *1 à 100% d'énergie)
- Refonte majeure du système de mise à jour du monde et des bâtiments :
	- Il ne devrait plus y avoir de bugs aux calculs des poucentages de ressources disponibles pour les bâtiments, et les factures d'électricité et d'eau sont maintenant correctement calculées et affichées
- Déblocage des bâtiments suivant la population : il est dorénavant impossible de construire un Grand building individuel dès le début du jeu par exemple, il faut atteindre un certain seuil de population pour le débloquer.
- Les fumées des bâtiments ne sont plus affichées lorsque le facteur de production de ces bâtiments est nul
- Pour les bâtiments non texturés (de couleur complètement grise dans les versions précédentes), des matériaux colorés leur ont été associés + Fin de l'architecture la pompe à eau (avec l'animation correspondante)
- Durant le chargement d'une partie sauvegardée, le manque d'un bloc XML (ex : <GUI> ... </GUI>) ou sa mauvaise position dans le fichier n'empêche plus le chargement de la partie
	(Attention : le support du mauvais ordre des blocs <Batiment> ... </Batiment> n'a pas été résolu (voir EcoWorldSystem.cpp : EcoWorldSystem::load, ligne 262 environ))
- Enregistrement du nom des bâtiments dans les sauvegardes au lieu de leur ID (permet un changement des ID des bâtiments sans automatiquement invalider toutes les sauvegardes déjà créées jusqu'à présent)
	et changement du nom de base des blocs des bâtiments de <Batiment123> en <Batiment> car ces nombres ralentissaient légèrement le chargement et n'étaient pas nécessaires
- Un arbre produit maintenant du bois à sa destruction proportionnellement à son âge (jusqu'à 1 an)

Pour le 03/05/12 : Version 0.0.0.0 :
- Meilleure gestion des factures d'eau et d'électricité dans les bâtiments, ajout de la gestion de la consommation d'électricité
- Refonte complète du menu options : mieux organisé, plus d'options, ajout d'une fenêtre de confirmation des paramêtres en cas de modification de paramêtres importants
- Meilleure transitions des temps, par l'utilisation d'un shader permettant l'interpolation entre 2 textures de ciel, et par la réécriture complète de la classe WeatherManager, qui fournit maintenant une gestion du temps plus claire et plus optimisée
- Support amélioré du Normal Mapping (résolution des bugs, seules quelques normal maps restent à créer)
- Animation des arbres par rotation (comme poussés par le vent)
- Ajout du module d'animation de la caméra suivant une ligne personnalisable par l'utilisateur (au moyen d'une fenêtre graphique) et de la possibilité d'enregistrer/charger cette animation depuis un fichier de sauvegarde
- Optimisation de la caméra RTS et mise à jour de ses animators et de ses enfants (dont la caméra FPS fait maintenant partie) avec le temps réellement écoulé, indépendament de la vitesse du jeu, et permettant aussi de déplacer la caméra lorsque le jeu est en pause
- Ajout des packs de musiques personnalisables, intégration des fichiers d'artiste (titre, logo, icône) de Romain DARCEL, et ajout d'un curseur de souris spécifique au jeu
- Terrain "Terres accueillantes.ewt" ajouté : Correspond aux instructions de terrains : Colline + Rivière + Mer + Zone de construction plane
- Lien dynamique de la DLL d'IrrKlang (irrKlang.dll) : Permet dorénavant de démarrer le jeu sans sa présence
- Passage à "Irrlicht SVN 1.8.0-alpha (révision 4020) Modified for EcoWorld"
- Possibilité de continuer la partie actuelle une fois gagnée
- Amélioration et optimisation de tous les shaders
- Scroll bar de sélection de la vitesse du jeu améliorée (+ Implémentation du mode Boost)
- Affichage d'une véritable date (ex : "25 juin 2003") pour le temps écoulé actuel en haut à droite de la GUI du jeu (le premier jour est "1 janvier 2000")
	+ Réaffichage du temps écoulé sous l'ancien format dans son texte de description

Pour le 09/11/11 :
- Objectifs
- Blocs de béton sous les bâtiments
- Textes de description des terrains + Créateur de terrain
- Enregistrement/Chargement des positions des fenêtres de l'interface du jeu
- Début de support du Normal Mapping (shader buggé actuellement, mais architecture globale terminée)
*/

/*
A voir avec Thomas :

- Revoir les valeurs des modifiers de difficulté pour équilibrer le jeu même à des difficultés inférieures



Anciennes questions :

- Production d'électricité : implémenter des Barrages hydro-électriques ? (grande production d'électricité)
	=> A oublier

- Remise à l'échelle des bâtiments :
	- Maisons et immeubles : Taille de base disisée par 3
		-> Problème pour le Chalet (taille non entière)
			=> Arrondir au-dessus
	- Buildings : Taille de base divisée par 2 seulement (pour obtenir des tailles entières) -> Contradiction avec la mise à l'échelle des maisons (mais échelle globale correcte)
		=> Valide

- Revoir les nouvelles couleurs des bâtiments non texturés

- Revoir les populations minimales nécessaires pour débloquer les bâtiments : StaticBatimentInfos::unlockPopulationNeeded
	+ Désactiver (voire masquer ?) les boutons de la GUI pour les bâtiments bloqués ?

- Echelle des bâtiments revue :
	- Taille des panneaux solaires un peu grande (comparé avec la maison avec panneaux solaires), mais ils ont déjà la taille minimale !
		-> Peu important
	- Certaines maisons devraient avoir une taille non entière (dû à leur redimendionnement) : elles ont actuellement la taille supérieure, mais leur bloc de béton est ainsi plus grand !
	=> Fait

- Nécessité de la valeur réelle de chaque ressource pour implémenter la vente et l'achat de ressources
	-> Pifomètre

- Note : Références :
	- M6 (journal télévisé) : 390 kg de déchets par habitant par an en moyenne
		-> Bonnes valeurs des déchets actuelles (1 kg / jour / habitant)
	- Professeur de Géographie : 85 kg de viande consommées par habitant par an en moyenne (en France) (=> 233 g / jour / habitant)
		-> Mauvaise valeurs actuelles (1 kg / jour / habitant)

- Affichage de la vériable date :
	- Afficher "1er janvier" au lieu de "1 janvier" ?
	- Les mois comportent tous 30 jours : Problèmes avec les 29/30 février et les mois de 31 jours !
	- Donne au jeu une dimension réelle : ne concorde pas tellement avec l'idée d'un monde (complètement vierge au départ) à construire
	- Les mois ne concordent pas avec le temps actuel (ex : il peu faire parfaitement beau en décembre !) : Gênant ?
		=> Peu important

- Modifiers : Prix des bâtiments modifiés -> Temps de construction/destruction modifiés aussi en conséquence ?
	-> Non

- Récupération des eaux de pluie ? (depuis que la pluie a été implémentée)
	-> Non : la pluie permet d'alimenter les ruisseaux

- Créer une liste des raccourcis claviers, des fonctionnalités du jeu, de ses options, de ses particularités et les consigner dans un Manuel Utilisateur

- Animer le fond du menu principal avec une "vidéo" (images animées logiciellement) :
	=> Avantages : chargement plus rapide (pas de création de terrain, seulement de textures), pas besoin de charger de bâtiments en avance, apparence professionnelle...
	-> Demander des propositions d'idées pour cette animation



- Que se passe-t-il lorsqu'un bâtiment a atteint la fin de sa durée de vie ?
	-> Il est détruit, mais quel effet cela a-t-il sur le budget, l'ES, les déchets... ?
- Que se passe-t-il pendant la construction d'un bâtiment (à part attendre qu'il soit construit, y a-t-il de la pollution rejetée) ?
	-> Pendant la construction d'un bâtiment, les ressources nécessaires sont-elles prélevées une fois pour toutes au début de la construction, ou petit à petit durant la construction ?
- Que se passe-t-il pendant la destruction d'un bâtiment (continue-t-il à rejeter des déchets et de l'ES, consommer des ressources) ?
	-> Pendant la destruction d'un bâtiment, les ressources nécessaires sont-elles prélevées une fois pour toutes au début de la destruction, ou petit à petit durant la destruction ?
- Représentation 3D de la destruction identique à celle de la construction !?
- effetSerreC et dechetsC sont rejetés à la fin de la construction du bâtiment ou lorsque l'ordre de construction est lancé ?
- effetSerreD et dechetsD sont rejetés à la fin de la destruction du bâtiment ou lorsque l'ordre de destruction est lancé ?
	-> Actuellement : au lancement des ordres

- Peut-on annuler la construction/destruction d'un bâtiment ? Quelles conséquences ?
- Que se passe-t-il lorsqu'on détruit un bâtiment pendant sa construction ?


- Quelles informations afficher dans le tableau récapitulatif des fins de mois ?
- Réorganiser l'ordre des informations dans la fenêtre d'informations et dans le tableau récapitulatif des fins de mois

- Doit-on afficher les ressources consommées par les habitants de la maison sélectionnée dans la fenêtre d'informations ?
	-> Actuellement : Non
- Doit-on aussi mettre à jour le loyer réel des maisons et la production réelle des usines (affichée dans la fenêtre d'informations) avec l'énergie disponible et la satisfaction des habitants ?
	-> Actuellement : Oui
- Dans la fenêtre d'informations, remplacer "Loyer" par "Loyer moyen", puisque dépendant d'autres facteurs externes (énergie, satisfaction)
- Revoir l'organisation des textes dans la fenêtre d'informations
- Dans la fenêtre d'informations, passer les "Déchets (J)" en "Déchets (M)" pour éviter la confusion avec l'"Evolution des déchets (M)" ? (idem pour effet de serre)

- Eviter la sélection des pans de route ?
- Renommer les onglets de la construction -> Réorganiser tous les boutons de construction
- Revoir les textes dans la fenêtre d'informations sur la destruction d'un bâtiment
- Organisation de l'écran "Perdu" à revoir


- Créer les fonctions pour les calculs des informations sur les bâtiments (à placer dans [Static]BatimentInfos::calculateFunctionsData())

- Recalculer les prix des maisons | immeubles | buildings individuel(les) / BC / avec PS / grand(e)s
- Feuilles de calcul pour les déchets, ES, ressources, prix... (une est déjà proposée)
- Augmenter les factures d'électricité des maisons (presque *3 !) -> Revoir les consommations d'eau et d'électricité ainsi que leurs factures
- Implémentation des derniers bâtiments indispensables : revérifier les valeurs
	Données des nouveaux bâtiments à revoir !

- Revoir les éoliennes : http://fr.wikipedia.org/wiki/%C3%89olienne#Poids_.C3.A9conomique_des_acteurs_de_l.27industrie_.C3.A9olienne
	-> Plus grandes précisions ici : http://fr.wikipedia.org/wiki/%C3%89nergie_%C3%A9olienne
		et ici : http://eolienne.f4jr.org/
- En réalité, hydroliennes sûrement beaucoup plus chères : http://fr.wikipedia.org/wiki/Hydrolienne
- Petites + Grosses hydroliennes ? (http://generationsfutures.chez-alice.fr/energie/hydrolienne.htm)
	-> Dans l'embouchure des fleuves / Dans la mer
- Hydroliennes complètement immergées ou dépassant à la surface ?
- Rentabilité des hydroliennes : 1 € / W
	-> Réalité : 2 turbines de 18m ! <=> 1 MW /h (http://generationsfutures.chez-alice.fr/energie/hydrolienne_illustration_ouestfrance.jpg)



Décidé :

- (Usine de plastique (maïs + eau))

- Ponts (500 € par m²) en pierre

- Passer des Watts aux MegaWatts/KiloWatts (plus réaliste et plus facile à manipuler) ? -> 4 kW par maison

- Achat de terrains (1km * 1km) -> 1 000 000 €
- Achat possible de 36 terrains
- Terrain de départ : 2km * 2km

- Stockage des ressources -> Créer un entrepôt

- Personnes travaillant -> Pour le moment : personne ne travaille !!!

- Maisons de départ

- Des gisements !!!

- ex : arbres -> bois (scierie)
gisement de fer -> fer (métallerie)
gisement de pierre -> pierre (fabrique de pierre)
gisement de sable -> sable (sablière)

- (((Recyclage ; Commerce entre villes ; Saisons (production différente))))

-> Tutoriel ! ; Scénarios ! ; Campagne ?

- Ce qui se produit lorsque les citoyens n'ont pas assez d'une ressource pour vivre (comme par exemple, l'eau) -> Les gens partent et ne payent plus
- Est-ce qu'on peut acheter ou vendre des ressources ? -> Oui
- Est-ce que les habitants consomment des matériaux de construction (pour rénover leur maison, se créer une petite cabane en bois, ...) ? -> Oui, après ils payent plus

- Le reste des ressources et des bâtiments, et les règles avancées du jeu (comme la recherche ou le recyclage)
- Centres commerciaux, Hopitals... -> Services municipaux et d'accueil aux personnes ?



Liste des bâtiments :
---------------------
- Maisons, immeubles, buildings, châlet : on garde ce qui reste actuellement

- Production des ressources (Usines) :
	-> Tant que les gisements n'auront pas été implémentés, les productions de base (ex : pierre, sable, calcaire...) pourront produire n'importe où sur la carte
{
	- Eau :
		- Extraction depuis les nappes phréatiques : Pompe d'extraction de l'eau (production de base) (débloqué)
		- Traitement de l'eau des ruisseaux/mers : Usine de traitement de l'eau (placée à côté d'un point d'eau) (production de base) (bloqué au départ)
	- Verre :
		- Usine de verre
	- Pierre :
		- Fabrique de pierre (production de base)
	- Métal (anciennement : fer) :
		- Métallerie
	- Bois :
		- Scierie (pas besoin d'arbre autour pour le moment) (production de base)
	- Ciment :
		- Usine de ciment
	- Tuiles :
		- Usine de tuiles
	- Sable :
		- Sablière (production de base)
	- Panneaux solaires :
		- Usine de panneaux solaires	<- A faire !

	- Quartz :
		- Exploitation de quartz (production de base)
	- Potasse :
		- Usine de potasse (pas besoin d'arbre autour pour le moment) (production de base)
	- Argile :
		- Exploitation d'argile (production de base)
	- Calcaire :
		- Exploitation de calcaire (production de base)

	- Viande :
		- Abbatoir de vaches
		- Abbatoir de moutons
	- Poisson :
		- Port de pêche (placée à côté d'un point d'eau) (production de base)
		- Pisciculture (production de base) (création d'un point d'eau pour l'occasion !)
	- Pain :
		- Boulangerie
		- Usine de pain
	- Huile :
		- Huilerie (nécessite du tournesol)	<- Nom officiel à revoir !
	- Vin :
		- Pressoir
	- Lait et Vaches :
		- Ferme de vache (nécessite de l'avoine)	<- Nom officiel à revoir !
	- Laine et Moutons :
		- Ferme de moutons (nécessite de l'avoine)	<- Nom officiel à revoir !
	- Pommes :
		- Verger de pommiers (pas besoin de pommiers autour pour le moment) (production de base)
	- Poires :
		- Verger de poiriers (pas besoin de poiriers autour pour le moment) (production de base)
	- Bananes :
		- Verger de bananiers (pas besoin de bananiers autour pour le moment) (production de base)
	- Oranges :
		- Verger d'orangers (pas besoin d'orangers autour pour le moment) (production de base)
	- Raisin :
		- Vignoble (pas besoin de vignes autour pour le moment) (production de base)
	- Tomates :
		- Culture de tomates (pas besoin de plants de tomates autour pour le moment) (production de base)
	- Haricots :
		- Culture de haricots (pas besoin de plants de haricots autour pour le moment) (production de base)
	- Pommes de terre :
		- Culture de pommes de terre (pas besoin de plants de pommes de terre autour pour le moment) (production de base)
	- Salades :
		- Culture de salades (pas besoin de plants de salades autour pour le moment) (production de base)
	- Carotte :
		- Culture de carottes (pas besoin de plants de carottes autour pour le moment) (production de base)
	- Blé :
		- Champ de blé (pas besoin de plants de blé autour pour le moment) (production de base)
	- Tournesol :
		- Champ de tournesol (pas besoin de plants de tournesol autour pour le moment) (production de base)
	- Avoine :
		- Champ d'avoine (pas besoin de plants d'avoine autour pour le moment) (production de base)

	- Vêtements :
		- Usine de textile (nécessite de l'indigo et de la laine)
	- Indigo :
		- Champ d'indigo (pas besoin de plants d'indigo (indigotier) autour pour le moment) (production de base)
}



Usines de recyclage :
- Ne recycle que ce qui ressort des usines et des habitations
	-> Les usines et les habitations rejettent des déchets recyclables
	-> Une usine de recyclage peut acceuillir un certain nombre de déchets recyclables, et les déchets recyclables qui ne peuvent pas être traités sont perdus en déchets

- (Centrale nucléaire ?	<- Nécessite des ressources et déchets nucléaires (à implémenter) !)



Terminé : (peut-être encore à revoir)
---------
- Réserver les impôts aux habitations (seuls bâtiments utilisant ce paramêtre) ?

- Afficher une véritable date (ex : "25 juin 2003") pour le temps écoulé actuel en haut à droite de la GUI du jeu ? (le premier jour serait "1 janvier 2000")

- Enregistrer les positions des fenêtres de la GUI dans les sauvegardes + Créer une touche pour réinitialiser la GUI du jeu (touche "C")

- Pouvoir arrêter les usines

- A partir de combien de dettes le joueur a-t-il perdu ? -> -100 000 € <- A revoir

- Durée de vie des bâtiments : (dont 20% aléatoire)
	Maison/Chalet 100 ans
	Immeuble 95 ans
	Buildings 90 ans
	Usine d'incinération des déchets 50 ans
	Usine (petite) 40 ans
	Usine (grande) 60 ans
	Centrale à charbon 30 ans
	Panneau solaire 10 ans
	Eolienne 70 ans
	Hydrolienne 60 ans
	Décharge 200 ans
	Scierie/Fonderie/Taileur/... 40 ans
	Arbre 150 ans (aléatoire : 40 %)

- Tps de construction/destruction
	- Coût de construction : prixC / 2
		-> 5000 €/ mois peuvent être construits (5 personnes à 1000 € par mois)
		==> 10 000 € / C <-> 1 mois de construction
		ex : maison individuelle : 60 000 € -> 6 mois de construction
	-> Destruction : tps de construction/10 ==> 10 000 € / D <-> 1 mois de destruction

- Eau : Matériau ou Nourriture ? -> Matériau !

- Fonctionnement actuel des loyers : à la fin de chaque mois, le loyer est perçu, que la maison existe depuis 20 jours ou qu'elle vienne juste d'être construite
	-> On laisse comme c'est

- Les ressources de départ et l'or de départ

- Que se passe-t-il lorsque l'énergie tombe en-dessous de 0 ? (par exemple : en créant des batiments de production d'énergie, en créant des batiments utilisant cette énergie, puis enfin en détruisant les batiments de production d'énergie)
	-> Peut-on détruire un batiment si on n'aura ensuite plus assez d'énergie ? Oui, mais le malus sera ailleurs

Décharges :
- Une décharge ne coûte pas cher, prend beaucoup de place et peut contenir beaucoup de déchets
- Les déchets disparaissent très lentement
- On peut recharger une décharge, jusqu'à un seuil maximum
- En détruisant une décharge, les déchets qu'elle contient reviennent dans le compteur de déchets, si elle n'en contient plus, la destruction d'une décharge ne coûte qu'un peu d'argent

Usines d'incinération des déchets :
- Les déchets entrants deviennent de l'ES (voir combien de déchets font combien d'ES)

Usines de production d'énergie :
- Eoliennes
- Champs de panneaux solaires -> Fait
- Centrales à charbon (de bois) (bois en entrée)
- Hydroliennes


- Des bonus de jeu suivant une difficulté choisie, permettant d'avoir plus d'argent et moins d'effet de serre et de déchets en mode Facile, et plus de taxes en mode Difficile
- Que se passe-t-il quand le joueur a beaucoup trop d'ES ? -> Taxe quadratique : 50k -> 1M
- Que se passe-t-il quand le joueur a beaucoup trop de déchets ? -> Taxe quadratique : 100k -> 2M



******************** Batiments de production d'énergie ********************

- Centrale à charbon :
Prix : 120000 (C) / 12000 (D)

5W <-> 1kg de bois / M <-> 0.08 € / M
300 000 W max => 60 000 kg de bois /M max !

- 1 Panneau solaire : 1W <-> 2.5 €
Prix : 1250 €
Production : 500 W

- 1 Eolienne : 1W <-> 2 €
Prix : 80000 €
Production : 40000 W de base (OU : 20000 W par 100 m au dessus du niveau de la mer avec 50 000 W max -> si possible !, à revoir)

- 1 Hydrolienne : 1W <-> 1.5 €
Prix : 90000 €
Production : 60000 W
*/

/*
Note sur les correspondances des tailles :
	2.0f sous Blender <=> 1.0f sous EcoWorldSystem <=> 40.0f (TAILLE_OBJETS) sous Irrlicht
*/

/*
Méthode pour ajouter un batiment :
----------------------------------
- Ajouter son ID dans Batiments.h -> BatimentID (attention : la position de l'ID dans l'énumération détermine aussi l'ordre de mise à jour du bâtiment dans sa liste)
- Ajouter ses caractéristiques dans Batiments.h -> StaticBatimentInfos::setID (toutes les caractéristiques ont des valeurs par défaut, certaines peuvent donc être omises) (ne pas oublier d'indiquer son nom !)
	/!\ : Si c'est une habitation, bien renseigner dans le début de la fonction calculateFunctionsData() la variable isHabitation !
	/!\ : Si c'est un arbre, bien renseigner dans le début de la fonction calculateFunctionsData() la variable isTree !
- Déterminer si ce bâtiment :
	- autorise le réglage de son pourcentage de production par l'utilisateur, en renseignant la méthode StaticBatimentInfos::needPourcentageProduction (facultatif : non par défaut)
	- nécessite un bloc de béton sous lui, en renseignant la méthode StaticBatimentInfos::needConcreteUnderBatiment (facultatif : oui par défaut)
	- peut être créé par rectangle de sélection, en renseignant la méthode StaticBatimentInfos::canCreateBySelectionRect (facultatif : non par défaut)
- Indiquer sa liste pour la mise à jour du monde dans EcoWorldSystem::addBatimentToLists
- Mettre à jour son comportement spécial face au monde dans Batiment::updateWorldFromBatimentID() (facultatif)
- Ajouter son bouton dans la GUI dans GUIManager.cpp -> GUIManager::createGameGUI (en utilisant la fonction GUIManager::addBoutonBatiment)
- Ajouter son modèle 3D dans EcoWorldRenderer.cpp -> EcoWorldRenderer::loadBatiment
- Ajouter son texte dans EcoWorldRenderer.cpp -> EcoWorldRenderer::loadTextBatiment
*/

/*
Méthode pour ajouter une ressource :
------------------------------------
- Ajouter son ID dans Ressources.h -> Ressources::RessourceID
- Ajouter son nom complet dans Ressources.h -> Ressources::getRessourceName et dans Ressources.h -> Ressources::getRessourceNameChar (les noms doivent être dans le même ordre que les ID)
- Ajouter son unité dans Ressources.h -> Ressources::getRessourceUnit (les unités doivent être dans le même ordre que les ID)
- Ajouter les coûts et les productions de cette ressource pour les batiments dans Batiments.h -> BatimentInfos::setID (facultatif)
- Ajouter la consommation de cette ressource par jour et par personne dans EcoWorldInfos.h -> update (facultatif)
*/

/*
Méthode pour ajouter un temps (weather) :
-----------------------------------------
- Ajouter son ID dans Weathers.h -> WeatherID
- Ajouter ses caractéristiques dans Weathers.h -> WeatherInfos::setID (toutes les caractéristiques ont des valeurs par défaut, certaines peuvent donc être omises)
- Créer son interpolation dans WeatherManager.cpp -> WeatherManager::calculateCurrentWeatherInfos en utilisant la macro INTERPOLATE si necéssaire
- Créer ses effets sur le device dans EcoWorldRenderer.cpp -> EcoWorldRenderer::updateWorldFromWeatherManager() (facultatif)
*/

/*
Note importante sur swprintf_s :
--------------------------------

Exemple valide :
{
	wchar_t str[50];
	swprintf_s(str, 50, "Exemple valide");
}

Exemple non valide :
{
	wchar_t str[50];
	swprintf_s(str, sizeof(wchar_t) * 50, "Exemple non valide !!!");
}

En effet, le second argument demandé par swprintf_s n'est en fait pas la taille mémoire du buffer en octets (comme on pourrait le croire à cause de son type : size_t),
mais en fait sa taille en nombre de caractères (soit deux fois moins si on considère que sizeof(wchar_t) = 2 octets) !

En mode DEBUG, cette différence peut occasionner des erreurs d'écriture en-dehors d'une zone mémoire allouée (bufer overflow),
car swprintf_s semble effacer complètement le buffer fourni. Cette erreur se traduit par erreur d'exécution à la fin de la fonction (accolade fermante finale) utilisant swprintf_s :
Run-Time Check Failure #2 - Stack around the variable 'str' was corrupted.

En mode RELEASE, le dépassement de capacité ne semble pas avoir été avéré (erreur non détectée (silencieuse ?), ou peut-être que contrairement à la version DEBUG, swprintf_s n'efface pas le buffer),
mais il reste toujours un bug possible, qui apparaît si plus de la moitié du buffer fourni est écrite.

*/

#endif
