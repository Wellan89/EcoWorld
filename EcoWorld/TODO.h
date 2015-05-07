#ifndef DEF_TODO
#define DEF_TODO

/*
TODO.h : Liste des t�ches � faire, ne pas inclure
*/

/*
BUGS Connus :
-placer les plus grandes res au d�but ; ne pas les s�lectionner lors de la cr�ation du menu options
- BUG : Echap dans le menu options avec la fen�tre de confirmation, puis retour dans le menu options
- BUG : Les versions 64 bits compilent mais plantent !
- Probl�me : les tests ne se lancent pas en mode DEBUG

- BUG : Crash du jeu lors du d�filement de la fen�tre d'informations avec la molette de la souris : Bug de la fonction DispatchMessage dans CIrrDeviceWin32::run() !

- BUG : Lors du chargement d'une partie (s�rement aussi lors des parties normales), la s�lection de b�timents ne s�lectionne pas les bons nodes
- BUG li� : Lors de la s�lection d'un b�timent gr�ce � son tour de construction,
	si c'est la face arri�re du tour de construction qui est vis�e (visible car le backface culling est d�sactiv�),
	c'est un autre b�timent (node pr�c�dent dans la liste des nodes) qui est s�lectionn� (BUG d'Irrlicht s�rement)
	=> Ces bugs viennent d'un bug dans la d�termination d'un node d'apr�s un triangle lui appartenant
- ==> BUG : La s�lection d'un b�timent gr�ce � son bloc de b�ton ne fonctionne pas

- BUG : Dans le shader de l'eau, les parties des b�timents en-dessous de l'eau ne sont plus bien align�s avec les parties au-dessus de l'eau, et les parties en-dessous de l'eau ne sont pratiquement plus visibles !

- BUG : Les blocs de b�ton disparaissent parfois du rendu lorsqu'ils sont sur le coin de l'�cran (mais toujours visibles) et proches de la cam�ra (en mode cam�ra FPS)

- BUG : Les ombres stencil ne fonctionnent plus du tout : elles ne fonctionne qu'avec l'eau avec shaders, et font m�me bugger l'affichage de la GUI du jeu

- BUG : Chargement d'un terrain par double-clic (parties sauvegard�es aussi ?) : Lorsque le terrain � charger a le m�me nom qu'un terrain d�j� existant dans le dossier de donn�es du jeu, ce n'est pas le terrain choisi qui est charg�, mais celui pr�sent par d�faut dans le dossier des donn�es

- BUG : Lorsqu'il pleut et que le nom au-dessus d'un b�timent est affich�, parfois la pluie perd sa transparence (�a le fait aussi parfois avec la fum�e) (avec le driver DirectX 9, peut-�tre avec d'autres driver)

- BUG : Diff�rentes incoh�rences lorsque le programme EcoWorld est lanc� "tout seul" (sans DLL � c�t� ni dossier de donn�es)
	-> Il semblerait y avoir des probl�mes de chevauchement m�moire dans ce cas l� ! (flagrant avec les hauteurs du terrain, qui sont compl�tement al�atoires ; ainsi qu'avec la r�solution, qui a du mal � passer en 800x600 : 16 bits m�me apr�s plusieurs clics sur le boutons appliquer)

- BUG : Lors du redimensionnement de la fen�tre de notes, les scroll bar ne sont pas automatiquement bien replac�es

- BUG : Le pourcentage de production d�sir� de la centrale � charbon (et peut-�tre des autres usines) est mal charg� depuis les sauvegardes
		+ La position de la barre du pourcentage de production dans la fen�tre d'informations n'est pas r�actualis�e lorsque la s�lection change de b�timent !
- BUG : La scroll bar de la fen�tre d'informations ne se remet pas automatiquement bien, au changement de b�timent (s�lection/destruction) (elle ne se remet pas en haut lorsqu'il y a la place pour afficher tout le texte)

- BUG : Lors du choix d'options non support�es par la machine actuelle + avec activation de l'audio :
			Le jeu reviens aux options pr�c�dentes (l'audio redevient donc th�oriquement d�sactiv�), mais l'audio reste activ� !
		=> BUG � rev�rifier !

- BUG : Petite coupure dans la texture des grilles de construction (sur la droite du panneau, tout au bord de la texture)



- BUG : Le soleil ne fait pas un tour par an, MAIS est � la bonne place lors du chargement d'une partie
	De plus, sa position ne concorde pas avec la direction de la lumi�re du soleil (SAUF lors du chargement d'une partie)
	=> Semble �tre r�solu (� rev�rifier)

- BUG : Le rendu est bugg� en mode OpenGL (les textures sont grises !) (sous le Toshiba Vista seulement : il fonctionne sur le nouveau Asus)

- BUG : (Sur ACER : Apparu lorsque le nombre de FPS < 10) : Les arbres perdent leur transparence (ils sont affich�s avec un fond noir) !

- BUG : OBBox : Ces bo�tes de collision ne fonctionnent actuellement pas !
- BUG : Maison cr��e (en construction) + cr�ation d'arbres en rectangles par-dessus (but : entourer la maison d'arbres) -> Les arbres sont aussi cr��s � l'int�rieur de la maison !

- BUG : Construction Map : Lors de son agrandissement, certains pixels en diagonale ainsi qu'en bord de carte deviennent noirs, emp�chant ainsi la construction (voir "Terrain.ewt" pour un aper�u du probl�me)

- BUG : Mode release : sortes "d'ombres vides" des b�timents sur le terrain cr�ant des trous dans lesquels on voit le ciel
	-> N'appara�t que lorsque l'anti-cr�nelage est d�sactiv� !
	=> Appara�t aussi avec DirectX 8 (au moins en mode Debug fen�tr�, mais avec un anti-cr�nelage de 2x)

- BUG : Probl�mes de collisions avec le triangle selector de la cam�ra FPS (la cam�ra s'arr�te toute seule au milieu du terrain, sans raison apparente, comme si la touche du clavier pour avancer avait �t� relach�e) (-> Bug remarqu� depuis l'impl�mentation des dalles en b�ton, mais ce bug appara�t m�me lorsqu'aucun b�timent n'est cr��)

- Probl�me : ToolTips : On voit parfois un tooltip d'un �l�ment masqu� lors du changement d'interface de jeu, et qui subsiste tant que la souris ne bouge pas (alors que ce tooltip n'�tait pas affich� lorsque son �l�ment parent �tait visible)

- Probl�me : Collisions : Lorsque la cam�ra FPS est sur un b�timent en construction et qu'il se termine, la cam�ra FPS reste bloqu�e dans le nouveau triangle selector

- Probl�me : Impossible de cr�er les b�timents de la pr�visualisation en rectangle en-dehors des limites terrain syst�me : on voit seulement le b�timent de pr�visualisation � la position point�e par la souris
	-> A rev�rifier : peut-�tre d�j� corrig�





En cours :

- Actuellement :
	=> Ombres de XEffects d�sactiv�es en mode Release (option du menu d�sactiv�e)
	=> S�lection des b�timents bugg�e (probl�me interne � Irrlicht !)



A voir rapidement :

- Lorsque le menu de construction descends automatiquement, attendre une ou deux secondes avant de le descendre une fois qu'il est en haut et que la souris vient de le quitter

- Probl�me : La version du fichier "EcoWorld (Kid version).exe" n'est pas "0.0.1.1" comme attendu (vient du compilateur de ressources)

- Revoir les �chelles de tous les b�timents :
		=> Toutes les tailles sont actuellement bonnes !
	+ Augmenter la hauteur entre la cam�ra FPS et le sol ?
	+ Revoir la position de d�part de la cam�ra RTS (� chaque d�part de partie je la d�place, m�me pour les tests rapides : c'est peut-�tre qu'elle n'est pas bien plac�e)
	=> Eviter les ajustements logiciels de la taille (blenderObjectsScaling * 0.5f) : les effectuer dans Blender directement
	+ Mettre � jour la valeur CAMERA_NEAR_VALUE et CAMERA_FAR_VALUE (� r�duire l�g�rement si possible)
	+ Revoir toutes les positions des textes au-dessus des b�timents et des fum�es pour v�rifier qu'elles sont toujours coh�rentes (en particulier pour les b�timents redimensionn�s sous Blender)
	+ Tourner les b�timents suivant de 180� dans blender :
		- Pompe d'extraction d'eau (90� seulement) (voir la position de la pompe d'extraction d'eau dans le jeu pour d�terminer quelle rotation est la meilleure : avec la roue en face de la cam�ra par d�faut)
		- Usine de verre
		- Usine de tuiles
		- Usine de papier
		- Centrale nucl�aire

- Redimensionn� : 1 carreau = 5 m�tres r�els	=> Revoir toutes les valeurs d�pendantes des surfaces (production de bl�...)
	- Centrale � charbon : x2	-> Agrandir fum�es
	- Hydrolienne : x2			-> Voir pour profondeur minimale de l'eau
	- Eolienne : x2
	- Maisons (habitations) : *2/3 (TODO)
	- Panneaux solaires

- BUG : Lors de la cr�ation d'une maison mesurant 6x3 unit�s :
	la maison est effectivement bien plac�e sur la grille en largeur (3 unit�s), mais elle chevauche des demi-unit�s sur la longueur (6 unit�s), elle serait bien plac�e si sa longueur �tait impaire
		=> Pouvoir placer correctement des b�timents de dimension paire
		=> Ou : N'avoir que des b�timents de dimension impaire ! (ajouter +1 � la taille du b�timent si sa taille est impaire)

- BUG : Le grand building individuel n'est pas en face de son bloc de b�ton !
	+ M�me probl�me pour les panneaux solaires

- Faire fonctionner les OBBox pour d�tecter les collisions lors du placement des b�timents

- Instructions sur les couleurs des mat�riaux des b�timents sous Blender (todo.txt)

- Cr�er les textures des Normal Map manquantes avec la nouvelle m�thode (avec leurs qualit�s diff�rentes) :
	=> Modifier la m�thode pour prononcer l'effet de Normal Mapping (ajouter des it�rations de normalisation gr�ce au plugin NormalMap (modifier la valeur Size � plus de 1) juste avant la Normal Map Finale)
		- Blocs de b�ton
		- Route
		- Panneaux solaires
		- Maisons
		- Chalet (� recr�er compl�tement !)
		- Immeubles (� recr�er compl�tement !)
		- Buildings (� recr�er compl�tement !)

- Cr�er les derni�res architectures 3D des b�timents + Colorer ces b�timents

- Pouvoir d�placer les b�timents
- Am�liorer les routes

- R�activer les ombres stencil (seule la lumi�re de ces ombres semble manquer lorsque l'eau avec shaders est utilis�e ; sans l'eau avec shaders le rendu est bugg� : on ne voit que le ciel et le soleil)

- Eau avec shaders : Ajouter un brouillard au fond de l'eau pour donner une impression de profondeur (la lumi�re devrait �tre plus rapidement absorb�e avant d'atteindre le fond)
	=> S�rement modifiable dans les shaders directement (mais risque peut-�tre d'entrer en conflit avec le brouillard automatique de DirectX)

- Gisements + Pouvoir choisir un terrain avec des arbres directement dessus
- Vente/Achat de ressources

- Utilit� d'une impl�mentation d'une destruction rectangulaire des b�timents ?
	=> Bouton Destruction -> Trac� d'un rectangle � la souris -> Relache le bouton gauche de la souris -> Question "Voulez-vous supprimez ces XXX �l�ments ?"

- R�organiser l'ordre de l'affichage des informations dans la fen�tre d'informations (au brouillon tout d'abord !) : actuellement non coh�rent !

- La vue de la mini-carte n'est affect�e ni par les effets post-rendu, ni par les ombres de XEffects

- Pour le bon fonctionnement de XEffects, il faut activer les lumi�res d'Irrlicht ! : Simplement passer la couleur de la lumi�re de XEffects � blanc pour ne pas att�nuer 2 fois les couleurs
- Ajouter d'autres types d'ombres depuis des shaders
	-> Impl�mentation de XEffects dans Game (Voir ici pour plus d'informations sur le Shadow Mapping : http://en.wikipedia.org/wiki/Shadow_mapping ; et : http://takinginitiative.net/2011/05/15/directx10-tutorial-10-shadow-mapping/)
		+ Tester le bon fonctionnement de l'activation XEffects depuis le menu options
		+ Activer aussi cette option en mode Release + Etendre ses possibilit�s (qualit� des ombres, li� � leur distance...)
		+ Dans EcoWorldRenderer::update : G�rer le cas o� initSunPosition est � true : red�finir la position r�elle de la lumi�re de XEffects (voir EcoWorldRenderer.cpp : ligne 3890)
		+ R�gler les param�tres Near value, Far value et Fov (la zone carr�e dans laquelle la lumi�re �claire) dans la cr�ation de la lumi�re pour les ombres
		+ Cr�er une CLightCamera d'apr�s les lumi�res des ombres pour permettre d'effectuer sceneManager->setActiveCamera(lightCam) avant le rendu de la profondeur dans la vue de la lumi�re
			-> Permet ainsi aux scene nodes qui sont rendus dans cette pass de s'optimiser suivant la vue actuelle
		+ V�rifier si on peut supprimer l'instruction shadowNode->OnRegisterSceneNode juste avant son rendu : en effet, cette instruction n'est pas cens� l'animer, et ralenti le rendu en l'ajoutant au scene manager (et risque aussi de l'afficher plusieurs fois dans le rendu du scene manager !)
		+ V�rifier si le temps de rendu est diminu� en changeant le statut des ombres du terrain de BOTH � RECEIVE (�vite un rendu suppl�mentaire du terrain), et celui des nodes (des b�timents) de BOTH � CAST
			-> Si le temps de rendu est effectivement diminu�, ajouter des options dans GameConfiguration pour pouvoir r�gler leur mode de r�ception/envoi des ombres (de BOTH � NONE pour chacun d'eux)
		+ Permettre � l'utilisateur d'ajouter des effets post-rendu (� travers le menu options) + Ajouter d'autres effets post-rendus personnalis�s en s'inspirant du SDK de DirectX 11 et des post-effets de Morrowind Graphic Enhancer (version GUI)
		+ Permettre (gr�ce � des defines du pr�processeur de shaders) de g�rer les lumi�res directionnelles plus efficacement dans XEffects (pas de position de lumi�re (seulement une direction), pas de distance max, pas d'att�nuation...)
		+ V�rifier que le rendu final des lumi�res dans XEffects et dans Irrlicht (de base) est identique !
		+ Shaders et callbacks de XEffects � optimiser (utiliser gl_XXX et envoyer les index des textures en OpenGL)
		+ Au lieu de compiler tous les shaders d�s la cr�ation de XEffects, compiler les shaders n�cessaires d�s qu'un node pr�voit de les utiliser (d�s la cr�ation d'une lampe par exemple : compiler les shaders avec le filtre sp�cifi� s'ils n'ont pas encore �t� compil�s)
			=> Acc�l�ration du chargement de XEffects et du menu principal !
		+ Bug � r�soudre entre XEffects et l'eau avec shaders
		+ V�rifier que les objets en-dehors de la shadow map sont bien rendus illumin�s et non pas en ombre
			+ BUG : Le normal mapping ne fonctionne pas avec XEffects puisqu'aucune lampe n'est assign�e au driver, de m�me pour les shaders de terrain Copland et N3xtD
				=> Il faut activer les lumi�res standards d'Irrlicht pendant la pass de rendu de la sc�ne, et n'utiliser la shadow map que pour assombrir la sc�ne !
		+ Etudier l'autre version de XEffects : OrthographicShadow (fonctionne avec les lumi�res directionnelles !)
		+ XEffects g�re mal la pr�visualisation des arbres (-> BUG lorsque l'arbre de pr�visualisation est d�truit) (peut-�tre parce qu'ils sont transparents ?)
		+ XEffects ne cr�e pas d'ombres sur les objets transparents
		+ R�soudre les probl�mes de culling dans la vue de la lampe (appuyer NumPad 1 pour un aper�u de la depth map)
		+ V�rifier que les animations ne sont pas 2x plus rapides avec XEffects
		+ Le brouillard semble �tre activ� m�me lors des rendus internes de XEffects (depth map et light map) sous DirectX
		+ V�rifier que XEffects ne d�sactive pas le brouillard (en OpenGL surtout)
		+ Am�liorer la pr�cision du depth buffer en utilisant : dist = tex.r + tex.g / 256.0f dans les shaders
		+ V�rifier que le d�placement dynamique de la lumi�re de XEffects fonctionne bien en mode cam�ra RTS et FPS
		+ Eviter l'appel � driver->getTexture (lent) pour obtenir les shadows maps (les stocker dans une core::map ?)
		+ D�sactiver tous les mat�riaux et les textures des scene nodes affich�s lors de la pass de rendu de la shadow map : optimisation du temps de rendu + d�sactiver les autres lumi�res...
		+ Animer les nodes avant les pass de rendu seulement lorsque n�cessaire
		+ Interdire le support des lumi�res ponctuelles (optimisation), et pouvoir directement indiquer light->setDirection plut�t que light->setTarget (le premier �tant le plus adapt� pour ces lumi�res)
		+ V�rifier que la direction de la lumi�re utilis�e est bien la bonne (indiqu�e dans RTSCamera::updateXEffects)
		+ Tester le r�sultat final en mode OpenGL et DirectX 9, et d�terminer les valeurs minimales PS_xxx et VS_xxx n�cessaires pour chaque shader (versions des PS et VS)
		+ Utiliser la valeur de la couleur des ombres des temps (WeatherInfos::shadowColor) pour d�terminer la couleur des ombres de XEffects
	-> V�rifier que TOUS les terrains re�oivent bien les ombres, ainsi que l'eau et les objets 3D eux-m�mes, et aussi les objets transparents (transparence ALPHA_REF surtout : transparence ALPHA non officiellement support�e, mais � tester tout de m�me si possible)
	-> GUI menu options � remodifier (Cr�er des onglets dans le menu options + l'adapter aux nouvelles options ajout�es)

	=> A noter dans la liste des am�liorations de version (Menu options am�lior� + Ajout d'options dans la configuration du jeu + Ajout des ombres de textures et des effets post-rendu gr�ce � XEffects)



- Impl�mentation des effets post-rendu :
	- BUG : La premi�re frame du jeu ne contient que le ciel (lorsque ses shaders sont activ�s) lorsque post process est activ�, si les shaders du ciel sont d�sactiv�s, la premi�re frame du jeu semble �tre d'une couleur al�atoire, proche de la couleur dominante du sol
	- Ajouter option calcul HDR (High Dynamic Range Lighting) + Autres effets divers de Morrowind (MGE et MGE XE)
		-> Voir ici pour de nombreuses informations sur les shaders : http://www.scribd.com/doc/72782053/73/Your-First-HDR-Shader-The-Glare
		(HDR : voir page 164)
	- Permettre l'envoi de donn�es suppl�mentaires aux shaders :
		- HDR (voir ci-dessus)
		- sunVec, sunCol, eyeVec, eyePos, sunVis ? (voir sunshafts.hlsl), rcpres ? (voir sunshafts.hlsl), waterLevel, fogRange, fov
			-> V�rifier automatiquement si les donn�es des HDR doivent �tre envoy�s aux shaders � chaque application des param�tres dans le menu options des effets post-rendus (V�rifier les donn�es n�cessaires pour chaque effet)
	- Diminuer tant que possible les versions des shaders utilis�es par PostProcess (id�al, m�me si impossible : tous les vertex et pixel shaders utilisent les versions VS_1_1 et PS_1_1)
		+ le shader de Depth inclus (pour la depth pass)
	- R�gler le format et la pr�cision de la texture RTT rttDepth : il est actuellement inutile qu'elle soit cod�e sur 32 bits RGBA
		alors qu'elle n'utilise que des nuances de gris dans la composante alpha (en effet les donn�ees des normales stock�es dans les composantes RGB de la texture ne sont jamais utilis�es) !

- BUG : Mode multijoueur : Lors de la construction de b�timents, la date de construction est rarement synchronis�e ! (lorsque le message prend plus d'un jour � �tre envoy� par exemple) (=> visible car le temps de vie des b�timents n'est pas synchronis�)
	=> Envoyer le jour du syt�me de jeu lors de l'envoi de chaque message pour que les clients sachent lorsque un message a �t� activ�, et puissent se synchroniser
- Version multijoueurs en r�seau :
	+ Permettre � l'utilisateur jouant en mode serveur d'envoyer les informations sur sa partie � un client (en utilisant l'ID ID_ADVERTISE_SYSTEM pour lui indiquer notre adresse et port) :
		/// RakPeer - Inform a remote system of our IP/Port. On the recipient, all data past ID_ADVERTISE_SYSTEM is whatever was passed to the data parameter
		ID_ADVERTISE_SYSTEM,
	+ GUI multijoueur � modifier pour permettre les parties sur Internet (adresse IP et port pouvant �tre indiqu�s par l'utilisateur)
	+ Permettre � l'utilisateur de jouer en ligne une partie sauvegard�e (qu'elle ait �t� jou�e en ligne pr�c�demment ou non)
	+ Sp�cialiser la construction de b�timents en rectangle, plut�t que d'utiliser plusieurs fois de suite la construction simple de b�timents (qui est tr�s co�teux en bande passante)
	+ G�rer la d�connexion des clients au serveur (bloque le jeu dans ce cas !) et inversement (notification), ainsi que leur connexion
		-> Cr�er un tableau affichant tous les clients multijoueurs et leur ping, ainsi que des boutons pour les �jecter de la partie (en permettant la simple d�connexion ou le banissement d�finitif (feature de RakNet))
	+ Organiser le PacketLogger actuel d'EcoWorld sous forme de tableau
	+ Utiliser le StringCompressor de RakNet pour l'envoi des parties sauvegard�es (voir StringCompressor.h et StringCompressor.cpp dans la source de RakNet)
		=> Impossible car "It only works with ASCII strings"

- Cr�er le b�timent de la scierie (sous Blender)
- Voir la TODO list des b�timents sous Blender (dans le dossier Textures)

- Utiliser les outils de Irrlicht i18n (outils de traduction, de font vectoriel...)



A faire (relativement rapide et/ou simple) :

- Optimiser l'utilisation de la m�moire de la version enfant (supprimer les variables inutiles de la GUI, �viter les trops grandes allocations de m�moire pour les ressources dans les informations statiques des b�timents, ...)

- Ajouter un type d'objectif au jeu : quantit� de ressource : la quantit� de ressources du joueur du type ressID doit v�rifier : ressID 'comparisonOp' dataF pour que cette condition soit v�rifi�e

- Tester intensivement le nouveau syst�me de mise � jour du monde et des b�timents dans des parties r�elles !

- Changer la couleur du fond de l'eau en fonction du temps : claire lorsque soleil, grise lorsque pluie...
	+ Avec l'eau avec shaders : augmenter le brouillard lors du rendu de la partie sous l'eau par rapport au brouillard en-dehors de l'eau pour faire l'effet de l'obscurit� naturelle de l'eau

- R�activer le mode 3D pour le son de la destruction du b�timent termin�e

- Diminuer la densit� des sons de l'eau (flagrant sur le terrain Highlands old !)
	-> Provient s�rement du fait que les �chelles dans Irrlicht ne sont pas les m�mes que sous irrKlang (1m != 1.0f !)
	+ Prendre en compte les positions de terrain en-dehors de la carte syst�me pour la d�termination du placement des sons de l'eau

- Mettre des couleurs r�alistes sur les usines (toit rouge, murs gris...)

- Tester l'utilisation du logo spatial � Romain DARCEL en tant que fond du menu principal
- Ajouter options fond du menu principal : soit : - Fond de chargement ; - Terrain plat (tourne autour) ; - Ville compl�te (animation)

- Diminuer les rendus pendant que le jeu est en pause pour une m�me position de la cam�ra en utilisant les RTT pour mettre le fond du jeu en cache, puis en affichant cette RTT gr�ce au screen quad � chaque rendu
	=> Seulement lorsque le menu Echap est affich� ! (trop de conditions � v�rifier autrement : b�timent s�l�ctionn�, b�timent point�, b�timent � construire, position et rotation de la cam�ra...)
		=> Abandonn� : ne vaut pas le coup !

- Cr�er un fondu entre les titres "EcoWorld" du menu principal pendant les changements de temps du menu

- Dans le texte d'informations, afficher les ressources manquantes pour construire le b�timent s�lectionn� en rouge

- Faire que la fum�e qui sort des b�timents soit proportionnelle au facteur de production actuel de ce b�timent
	+ Ne pas directement "masquer" la fum�e des b�timents d�s que leur facteur de production s'annule : faire que les derni�res particules de fum�es continuent � monter
		=> Incompatible avec le mode de gestion actuel de la fum�e : un seul syst�me de particules pour plusieurs chemin�es

- Cr�er une option dans les terrains pour qu'EcoWorld n'utilise pas la Construction Map fournie, mais qu'il la calcule automatiquement d'apr�s les hauteurs du terrain (non constructible lorsque : terrain sous l'eau ou assez proche OU fort d�nivel� ; constructible sinon)
	(comme il calcule d�j� si de l'eau profonde est disponble ou non)

- Impl�menter la r�fraction de l'eau (eau avec shaders) (les objets dans l'eau ne doivent pas �tre align�s avec les objets hors de l'eau, d� � la r�fraction de la lumi�re dans l'eau)

- Dans GameConfig et le menu options, et en utilisant le pr�processeur de shaders de XEffects : permettre d'activer ou d�sactiver la r�flexion de l'environnement sur l'eau avec shaders (car cela n�cessite un rendu suppl�mentaire � chaque frame, pour un effet relativement peu visible)

- Recompiler toutes les librairies et impl�menter les modes de compatibilit� et les modes 64 bits (cr�er des options sp�cifiques dans chaque projet)
	+ V�rifier les nouvelles versions de toutes les librairies utilis�es

- Ajouter une option pour effectuer le rendu en 3D (filtres bleu/rouge) : voir http://irrlicht.sourceforge.net/forum/viewtopic.php?t=33463

- Mettre � jour compl�tement la liste des parties multijoueurs r�guli�rement (toutes les secondes par exemple), pour �viter que des parties quitt�es par le serveur ne restent encore dans la liste

- N'activer la construction r�ctangulaire qu'apr�s 200 � 300 ms d'appui sur le bouton gauche de la souris, pour permettre par exemple de cr�er un arbre simple rapidement
	+ Activer la construction rectangulaire pour tous les b�timents ?

- Cr�er une option dans GameConfiguration pour remplacer le define USE_VARIOUS_TREES + Cr�er une case � cocher dans le menu options pour ce param�tre

- Revoir la pluie de Spark pour qu'on ne puisse pas voir le d�part des gouttes d'eau (elles tombent d'abord verticalement, puis se redressent peu � peu suivant une ligne oblique)
	-> Faire qu'elles tombent suivant une droite d�s leur d�part jusqu'� leur arriv�e, et non pas en tra�ant une parabole "sous l'action du vent"

- Cr�er plusieurs niveaux de zoom dans la mini carte

- Cr�er un bouton dans la GUI du jeu pour mettre le jeu en pause
- Cr�er un bouton dans la GUI (dans le menu de droite) pour permettre d'afficher le menu d'Animation de la cam�ra

- Lorsque RakNet ne peut �tre cr�� (dans le menu multijoueur ou lors de la cr�ation d'une partie multijoueur), afficher un message � l'utilisateur (dans le jeu, pas dans le log) indiquant clairement pourquoi RakNet n'a pu d�marrer (d'apr�s le code d'erreur qu'il a retourn� lors de sa cr�ation)
	+ A faire aussi lorsque c'est la connexion � un h�te (ou d'un client au serveur (joueur actuel)) qui a �chou�e

- Utiliser un tribool pour Game::vitesseMenuInferieur � la place d'un float actuellement

- R�tablir la transparence de la barre sup�rieure de la GUI du jeu

- Faire des tests sur les shaders lorsqu'ils ne sont pas support�s (possibilit� de simuler une feature non support�e par le driver : d�sactiver PS_2_0 ou VS_2_0 et sup�rieur, et v�rifier que le jeu ne plante pas)

- Ajouter les lumi�res sp�culaires au shader de l'eau actuel (donne un effet magnifique avec les reflets du soleil !)
	+ Permettre le rendu de l'eau avec shaders sans r�flection/r�fraction et sans lumi�re sp�culaire (suivant des options dans GameConfiguration), en utilisant le shader preprocessor de XEffects dans les shaders de l'eau

- Cr�er des options dans GameConfig pour g�rer :
	- La qualit� des syst�mes de particules (multiplicateur du nombre de particules �mises), plus une option pour les d�sactiver compl�tement (pas d'appel � SparkManager::init() dans ce cas)
	- La qualit� des arbres (multiplicateur de chacun des param�tres des niveaux de d�tail (highLOD et midLOD g�r�s s�par�ments !) + multiplicateur du nombre de feuilles)
		-> Bug avec les arbres : le d�but du tronc n'est pas en co�ncidence avec le reste de l'arbre (n'appara�t qu'avec des niveaux de d�tail bas)
		-> Pouvoir diminuer le nombre de feuilles des arbres
	- La qualit� des terrains (multiplicateur (x2, x4, x0.5, x0.25) de la taille de la height map utilis�e pour cr�er le terrain)
	- Le format d'enregistrement des captures d'�cran (bmp, jpg, png...)

- Transformer le tableau des ressources IGUITable ? (avec des boutons en face de chaque ligne pour l'achat/la vente de la ressource)

- G�rer les cas dans Game::render o� driver->beginScene et driver->endScene �chouent (ils retournent alors false)

- Revoir la position de tous les textes au-dessus des b�timents

- Faire que la touche "Back space" ait le m�me effet que la touche Echap dans les menus

- Am�liorer la fum�e 2 (-> de la centrale � charbon et de l'usine d'incin�ration des d�chets)
- Utiliser des syst�mes de particules pour la cr�ation/destruction d'un b�timent
- Revoir le mod�le de particules de la pluie

- BUG : A leur placement, les b�timents (bien qu'�tant � la position exacte de leur pr�visualisation) ont souvent une partie de leur zone de construction "sous le sol",
		malgr� la s�curit� prise par EcoWorldRenderer::getNewBatimentPos de calculer la hauteur maximale du terrain sur la surface du b�timent
	-> BUG Corollaire : Certaines routes se retrouvent actuellement compl�tement en-dessous du sol (voir partie "Thomas (Grande).ewg")

- BUG : On ne voit parfois pas tout le texte de la description des objectifs dans la liste modifiable des objectifs : Trouver un moyen de le faire d�filer
	-> L'englober dans un �l�ment parent CGUIScrollListBox contenant une IGUIListBox et une IGUIScrollBar horizontale pour la faire d�filer horizontalement

- BUG : Quelques petits bugs de passage � la ligne des textes sous certaines r�solutions => Utiliser les font vectorielles !
	-> Exemple : En 1024x768 : Informations de la satisfaction ("Satisfaction : 100 %\r\nSatisfaction r�elle : 100 %") : Passage � la ligne au dernier "%" !



Modifications � apporter � Irrlicht :

- BUG : Les accents dans les noms de fichiers ne sont pas support�s dans la liste de la fen�tre d'enregistrement/chargement de partie,
		mais ils sont support�s dans son edit box du nom de fichier (ne vient donc pas de la police utilis�e !)
	=> Vient du syst�me de fichier d'Irrlicht qui ne supporte pas les caract�res sp�cifiques wchar_t dans les noms de fichier
		-> Peut �tre r�solu en passant le syst�me de caract�res d'Irrlicht pour les noms de fichiers en wchar_t



Important (jouabilit� et possibilit�s de jeu pour le joueur) :

- Ins�rer les derni�res options de GameConfig dans la GUI du menu options : voir CGUIOptionsMenu.h

- Cr�er plusieurs curseurs pour le jeu (Irrlicht peut g�rer les curseurs syst�me : voir ICursorControl.h)

- Dans le menu d'animation de la cam�ra RTS, permettre au joueur d'ins�rer des points o� il veut, et de d�placer les points existants dans la liste (monter ou descendre le point s�lectionn�)

- Dans le module d'animation de la cam�ra RTS, permettre aussi de faire suivre un chemin personnalis� au point cible (target) de la cam�ra
	-> Cr�er un animator personnalis� pour g�rer une cam�ra et d�placer sa cible en m�me temps que sa position

- Permettre au joueur de r�gler pr�cis�ment le budget et les ressources de d�part pour une partie

- Ajouter des bruits de fond d'environnement (oiseaux, plus de vent (vari�t� des sons)...)
	+ Les sons manquants dans l'IrrKlangManager

- Impl�menter la destruction rectangulaire des b�timents (dans BatimentSelector et aussi dans RakNetManager)

- Cr�er une file de messages de jeu � gauche de l'�cran (style Anno 1404) permettant d'informer le joueur de chaque action importante (destruction d'un b�timent, manque important d'�nergie...)

- Impl�menter la vente/l'achat de ressources
	-> N�cessite la valeur r�elle de chaque ressource

- Cr�er le tableau r�capitulatif des fins de mois tel que pr�vu sur papier, et renommer le tableau actuel en "Liste d�taill�e des b�timents"
- Impl�menter des graphiques : budget/ES/d�chets/habitants... en fonction des jours/mois/ann�es

- Impl�menter les derniers b�timents

- Objectifs : Informer le joueur lors d'un objectif r�ussi/perdu
	+ Impl�menter les objectifs facultatifs

- Menu partie gagn� : Afficher un tableau des objectifs remport�s/perdus (+ leur date de derni�re r�alisation)

- D�bloquer les b�timents au fur et � mesure d�pendant de la population actuelle
	=> Lorsque la population retombe en-dessous du seuil minimal pour ce b�timent : rebloquer le b�timent ?

- Impl�menter les sc�narios (jeu d�j� cr�� + terrain associ� �ventuellement) ; Archive zip sous extension "ews" contenant un fichier "Scenario.xml" avec les informations sur le sc�nario
	-> Difficult� r�glable gr�ce aux modifiers (modifiers personnalis�s dans les sc�narios), mais aussi possible gr�ce au chargement de diff�rentes parties (plus ou moins faciles) suivant la difficult�
	-> La difficult� peut aussi �tre forc�e � certaines valeurs (ex : seulement Normal ; Normal ou Difficile...)
	-> Image de pr�visualisation du sc�nario + Texte de description

- De m�me, impl�menter les campagnes (Archive zip sous extension "ewc") :
	ensemble de sc�narios ayant un ordre et (en option r�glable dans la campagne -> Peut aussi �tre d�sactiv�) un niveau de difficult�

- Revoir les routes : ne plus les utiliser en tant qu'objets 3D mais en tant que texture rajout�e par dessus la texture de couleur du terrain (-> Peut-�tre utiliser un shader)
	-> Utiliser la m�me m�thode pour dessiner la grille du terrain

- Pouvoir acheter des terrains (nombre limit� et fix� : 36 terrains achetables)
	-> Support dans EcoWorldSystem ; Menu achat de terrain ; Adaptation de la mini-carte par rapport aux terrains achet�s
- Impl�menter les gisements (fichier dans le terrain "Gisements.xml" indiquant leur type et leur position) en 3D et dans le syst�me de jeu

- Tas de gravat cr��s � la place d'un b�timent d�truit par fin de vie

- Cr�er un fichier 3D avec des bou�es pour les zones de construction sur l'eau

- R�organiser le menu de construction pour qu'il prenne moins de place sur l'�cran
	+ Le r�organiser en plusieurs sous-�crans accessibles par des boutons fl�ch�s
	+ Simplifier grandement l'ajout de nouveaux boutons gr�ce � une grille pr�d�finie

- Cr�er un bouton pour afficher/masquer la grille du terrain
- Permettre de r�duire la barre de r�glage de la vitesse du jeu

- Avanc� : Cr�er une console (style Morrowind) pour avoir le contr�le complet sur le syst�me de jeu



Important (g�n�ral) :

- Lors de la construction de b�timents :
	si le b�timent � construire ne peut �tre cr��, faire dispara�tre progressivement le(s) b�timent(s) de pr�visualisation utilis� pour sa pr�visualisation (avec une transparence alpha progressive : dans le shader de NormalMapping),
	sinon, s'il peut �tre cr��, faire appara�tre un nuage de poussi�re (gr�ce � Spark) � l'endroit o� il a �t� implant� (comme dans Zoo Tycoon 2 quand les b�timents appara�ssent instantan�ment)				

- Utiliser un mesh combiner pour am�liorer les performances du jeu lorsque de nombreux nodes ont �t� cr��s (peut par exemple �tre utilis� pour combiner plusieurs arbres ou routes entre eux, acc�l�rant ainsi leur calcul)

- Permettre l'acc�s au menu options pendant le jeu

- Cr�er des param�tres pour pouvoir param�tres les touches de contr�le de la cam�ra et les touches de raccourcis

- Cr�er une nouvelle eau gr�ce � des shaders provenant de XEffects (voir Exemple4)

- Cr�er un header dans les sauvegardes ECW pour, lors du chargement des parties sauvegard�es, pouvoir r�ellement d�terminer si ce sont bien des sauvegardes EcoWorld

- Cr�er une classe GameLoader (pour la cr�ation des nouvelles parties, le chargement de parties sauvegard�es, la gestion des versions des sauvegardes (!), de sc�narios, de campagnes...)
	Ceci permettrait de d�charger la classe Game de plusieurs centaines de lignes de code
- Cr�er une classe CGUIUpBar g�rant la barre sup�rieure de la GUI du jeu, CGUIRightBar g�rant la barre de droite de la GUI du jeu et permettant de masquer la barre de commande de la vitesse du jeu, CGUIConstructionMenu g�rant le menu inf�rieur de construction pour d�charger Game de la gestion de ces entit�s
- Cr�er une classe pour chaque type de GUI du jeu : CGUIMainMenu, CGUIOptionsMenu, CGUINewGameMenu, CGUIGameInterface, CGUILoadingScreen (d�j� cr��e)

- Faire que les fonctions de cr�ation d'une nouvelle partie retournent un bool, et retournent true et affichent un message d'erreur lorsqu'une erreur survient lors du chargement du jeu
	-> Faire de m�me pour les sous-fonctions importantes de celles-ci (ex : EcoWorldTerrainManager::loadNewTerrain)

- Rendre les infos sur la position des fen�tres de la GUI ind�pendantes de la r�solution choisie dans les options !

- Revoir les sens des b�timents cr��s dans Blender et � leur cr�ation dans EcoWorld, ainsi que leur taille finale, leur agrandissement et leur position (certains ne sont pas centr�s)
	-> Utiliser TAILLE_OBJETS = 1.0f !
	-> Taille des maisons + usines � multiplier par 2
		-> Revoir la taille relative des �oliennes et des hydroliennes
	-> TAILLE_CARTE vaut 6000 pour un terrain de d�part de 2km + 4 terrains achetables de 1km
		-> Diviser les cartes en plusieurs sous-parties
	-> R�gler aussi les informations de la cam�ra d'apr�s ces nouvelles valeurs, et surtout le Target.Y (il est d� au bug qui semble faire tourner la cam�ra autour de sa position dans des terrains o� la distance en hauteur entre son target et le terrain est tr�s grande)

- Dans le tableau r�capitulatif : cr�er une ligne de Total
- Dans le tableau r�capitulatif : ne pas replacer la scroll bar en haut � chacune de ses mises � jour, et conserver s�lectionn�e la ligne actuelle
	-> Eviter d'effacer puis de recr�er toutes les lignes du tableau � chaque mise � jour (de plus : le syst�me actuel est tr�s long !)

- Pouvoir enregistrer les miniatures des jeux avec leur fichier d'enregistrement (une capture d'�cran qui serait visible lorsqu'on s�lectionnerait le jeu dans la liste des fichiers)

- Utiliser la CGridSceneNode pour l'affichage de la grille du terrain
- Permettre en appuyant sur une touche du clavier de voir le terrain constructible/non constructible/en eau profonde (seulement possible en mode DEBUG actuellement -> rendre l'affichage de la grille plus compr�hensible, ainsi que plus optimis�)
- Pouvoir voir les limites du terrain et le terrain achetable
	-> Pouvoir acheter du terrain

- (Ne pas cr�er toutes les GUI d'un coup lors de l'appel au GUI Manager : cr�er les GUI du jeu (Jeu et Jeu perdu) seulement lorsque n�cessaire (pendant le chargement principal du jeu))

- Rendre SPARK ind�pendant du device (comme IrrKlang : pouvoir changer de driver sans le remettre compl�tement � z�ro)

- Optimiser la classe obbox2df

- Cr�er une cam�ra pour une vue en mode "carte postale" comme dans Anno 1404



A faire (qualit� professionnelle et compl�te du jeu) :

- Cr�er d'autres terrains (les dessiner avant sur papier)
	-> Cr�er un terrain sablonneux (majoritairement), un terrain aux couleurs tropicales (couleur �meraude majoritairement), un terrain dans les neiges, un terrain sur un volcan
- Cr�er d'autres fonds d'�cran de chargement, plus r�cents et professionnels
	-> Ils devraient avoir une r�solution de 1920x1080 (Full HD)

- Utiliser un outil de localisation (traduction) vers l'anglais et/ou d'autres langues pour traduire les cha�nes de caract�res en langue fran�aises du code
	-> Utiliser GNU gettext si possible

- Permettre de modifier les touches par d�faut du jeu (impl�mentation gr�ce � un tableau touches -> actions (ACTION touches[KEY_COUNT];) + Fonction OnEventAction(ACTION act) g�rant les diff�rentes actions possibles)

- V�rifier tous les fichiers externes utilis�s, ainsi que tous les morceaux de code emprunt�s
- Cr�er un "copyright" ainsi qu'une license pour le jeu (pour autoriser sa diffusion : pr�vue vers juin 2012)
	-> Pr�voir une version payante du jeu (quelques euros ? (prix vus sur le forum d'Irrlicht : un jeu vaut environ 10~15� : il pourrait facilement �tre vendu entre 2 et 5 �)

- Cr�er un syst�me de mise � jour automatique (avec le module auto-patch de RakNet)

- Cr�er une vid�o d'introduction avant d'arriver au menu principal (vid�o de d�monstration + faisant office de "cr�dits")
	-> Utiliser Bink (RAD Video Tools) (avec une version plus ancienne mais sans pub ? => Non car ill�gal : emp�cherait la diffusion du jeu)

- Animer le fond du menu principal (avec la vid�o de fond)

- Cr�er un setup/launcher pour installer le jeu, DirectX 9 (DirectX 8 �tant d�pr�ci�), et g�rer son lancement automatique depuis un CD-Rom (affichage d'un menu et d�t�ction de l'installation actuelle)
	-> A faire

- Cr�er un �diteur de terrain et de parties sauvegard�es, sc�narios, campagnes
	-> Lorsque le projet sera termin� (A faire pendant les vacances d'�t� 2012 ?)

- Quitter la librairie d'Irrklang ? (relativement limitante, en termes de droits, d'options (non open source)...)



A faire (divers) :

- En mode multijoueur, permettre au serveur de n'accepter des clients qu'en mode observateur (ils ne peuvent produire aucune action, seulement regarder la ville)

- Essayer d'utiliser les joysticks pour contr�ler le curseur de la souris
- Permettre au joueur de choisir le joystick � utiliser (par une liste des joysticks d�tect�s dans le menu options)

- Modifier les curseurs affich�s suivant la commande actuellement s�lectionn�e (normal, construction, destruction, b�timent point�, rotation de la cam�ra...) (possible par l'envoi directement dans EcoWorld de messages � la fen�tre Windows affich�e)
	-> Cr�er un CursorManager pour g�rer tous les curseurs du jeu, et supprimer les curseurs des ressources du jeu
	+ Utiliser des curseurs anim�s (voir http://www.codeguru.com/forum/showthread.php?t=444335)

- Mieux g�rer les exceptions C++ qui pourraient arriver : voir http://www.codeproject.com/KB/exception/excv2.aspx

- Feature de replay ? (comme dans The Sleepless : enregistrer le temps syst�me quand chaque action a �t� effectu�e et les param�tres de l'action)
	=> Dans ce cas, �viter les documents XML : tr�s lourds ; pr�f�rer de simples documents textes, avec une action par ligne

- Faire que la cam�ra RTS ne puisse pas traverser les b�timents

- Tester une version portable (sur Android/IPhone)

- Cr�er une fen�tre indiquant la consommation des ressources des habitants et l'influence de chaque ressource sur leur satisfaction

- Trouver un meilleur moyen d'animer les arbres que de les agrandir/r�tr�cir lin�airement

- Dans la fen�tre d'informations sur le b�timent s�lectionn� : indiquer le loyer/facture d'�lectricit�/facture d'eau maximal possible (ex : "Facture d'eau (M) : 500 � / 2000 �")

- Destruction (voire s�lection ?) de b�timents en rectangle
- S�lections multiples ?

- Tableau r�capitulatif des fins d'ann�es/de mois : cr�er un affichage possible pour les mois pr�c�dents
- Remplacer le texte d'informations par un tableau (avec possibilit� de mise en forme) : un IGUITable

- Il y a quelques bugs dans le placement des batiments : pr�s des arbres (ou pr�s du bord du terrain ?), une erreur "place insuffisante" est lanc�e alors que la place est libre
- Dans la cam�ra RTS, zoomer en utilisant le FOV de la cam�ra, et non pas en rapprochant/�loignant la cam�ra

- On voit peu le titre lorsque le temps du menu principal est � 3 (WI_raining) (ce titre-ci est peu visible avec tous les temps : Illumin� Jour)
- Petites coupures verticales dans le titre du menu principal � supprimer

- Les hydroliennes fix�es au sol doivent toucher le fond !

- Pouvoir indiquer le type de brouillard dans WeatherInfos ?

- Permettre que le r�glage dans les options du jeu des diff�rents niveaux de qualit� de texture soit appliquable aussi dans les terrains

- Ne pas pouvoir modifier la rotation des hydroliennes/�oliennes pour les conserver face aux courants marins/au vent (-> qui ont tous les deux la m�me direction)




- Rechercher une meilleur cam�ra FPS (la cam�ra FPS d'Irrlicht est d�crite pour les tests, comprendre pourquoi (lente, peu pr�cise, bugg�e... ?))

- V�rifier le comportement du jeu avec des terrains plus petits que la taille de la carte syst�me
- L'erreur "Hors des limites du terrain" doit-il aussi prendre en compte le terrain 3D ?

- Prot�ger la construction en zone rectangulaire lorsqu'une zone est trop grande (la consommation m�moire devient alors �norme !)

- Le triangle selector du terrain Paged ne fonctionne pas !!!
	-> Gros bugs dans le terrain Paged, le rendant injouable !

- Pouvoir indiquer la taille du monde syst�me (TAILLE_CARTE et TAILLE_OBJETS) dans les informations du terrain (et les prot�ger avec un maximum et minimum !)
- Pouvoir indiquer les informations de la cam�ra dans les informations du terrain (sa position de d�part notamment)
- Cr�er une nature map dans les terrains (ou l'incorporer � la construction map) indiquant les gisements et la nature de d�part (surtout les for�ts), ainsi que les b�timents de d�part
- Lors de la cr�ation des terrains, ne cr�er le triangle selector que pour les triangles qui sont � l'int�rieur de l'espace du monde

- Bug de la loading bar en OpenGL
- Bug "Unsupported texture format" en OpenGL avec shaders de l'eau
- En OpenGL, la texture du ciel n'est pas bien plac�e sur le skydome

- Utiliser Boost pour �crire dans les fichiers XML
- Utiliser Boost pour supprimer les fichiers (au lieu de "remove")

- Rapprocher la couleur du brouillard de la couleur du ciel dans la gestion des temps (Weathers.h)

- Cr�er des outils pour la modification de certaines options du jeu : un EcwTerrainCreator pour cr�er/modifier les terrains, et un EcwConfigEditor pour modifier certains param�tres avanc�s de "Game Configuration.xml"

- Gros probl�me dans l'ombre du grand building individuel !

- Indiquer les nouvelles tailles des b�timents
- Normaliser les tailles des batiments pour que 1.0f <-> 1m r�el
- Eviter les tailles de b�timent impaires ?

- Commencer l'impl�mentation des b�timents finaux

- Les ombres stencil des �oliennes ne sont d�pendantes que de la rotation de l'�olienne (et pas de la position de la lumi�re des ombres)
	-> D� � un probl�me de normales des �oliennes
- Les ombres stencil de l'h�lice des �oliennes ne sont pas affich�es
- Les normales des �oliennes sont tourn�es de 90� en X (afficher les donn�es de d�bogage sur les �oliennes pour plus de pr�cisions)
- Les animations des �oliennes comportent des coupures vers la fin de leur animation (ces coupures ne sont pas dues au changement de vitesse de leur animation)

- Pouvoir d�truire les batiments par glissement ou par rectangle de s�lection

- Diminuer les r�p�titions de code dans les m�thodes placerBatiment

- Cr�er un nouveau timer gr�ce � Boost pour g�rer seulement le syst�me de jeu, et ainsi acc�l�rer le jeu sans acc�l�rer aussi les animations d'Irrlicht (scene nodes, animators et cam�ra)
- Utiliser Boost pour cr�er des threads diff�rents (1 pour les calculs du syst�me de jeu et des particules, et 1 pour le rendu Irrlicht)

- Les info-bulles de la GUI ne fonctionnent pas si le timer d'Irrlicht est arr�t� (lorsqu'on est en pause par exemple)

- Ajouter d'autres sons

- Faire que quand on est en pause, on puisse encore bouger la camera
	-> Rendre la cam�ra RTS ind�pendante du timer d'Irrlicht

- Remettre le CGUIFileSelector au propre
- Remettre la cam�ra RTS au propre (supprimer toutes les variables inutiles, optimiser les fonctions...)
- Revoir les pourcentages de chargement

- Le bouton pour la maison en bois n'est pas bien plac� dans la GUI (il est dans l'onglet des centres)

- La lumi�re pour les ombres stencil n'est pas vraiment � l'infini, r�soudre ce probl�me avec une lumi�re directionelle (impossible actuellement, attendre une prochaine version d'irrlicht permettant les ombres avec des lumi�res directionnelles)

- Il reste encore quelques bugs mineurs entre la transition entre la cam�ra FPS et la cam�ra RTS

- Cr�er un bouton de test du volume du son sous chaque Scroll Bar pour r�gler le son

- Id�e pour le fond du menu du jeu : on se balade dans une grande ville �cologique qui se construit sous nos yeux

- Chercher des musiques et des sons correspondants plus au type de jeu et � l'�poque actuelle

- Le LOD ne fonctionne pour le moment que pour la maison de test : l'impl�menter compl�tement

- On ne peut pas enregistrer des types "long" dans les fichiers, essayer de r�soudre ceci avec des pointeurs
- Dans GameConfiguration::save() et GameConfiguration::load(), utiliser des enum � la place des int quand c'est possible

- Utiliser des polices qui varient suivant la r�solution (peut-�tre attendre les polices vectorielles)

- R�organiser l'ordre des variables et fonctions dans Game.h
*/

/*
Id�es Am�liorations Thomas :
- Gisements
- Terrains de d�part : ajouter des arbres

- D�bloquer les b�timents au fur � mesure (par rapport � la population actuelle)
	-> B�timents diff�rents produisant la m�me ressources mais avec une rentabilit� diff�rente (ex : cabane de p�cheurs -> port de p�che -> pisciculture)

- Augmenter la zone non constructible des terrains actuels pr�s des rivi�res + augmenter la taille des terrains

- Emp�cher la cam�ra FPS de tomber en-dessous d'une certaine valeur + Indiquer un zoom maximum r�el pour la transition entre FPS -> RTS

- B�timent d�truit par fin de vie : tas de gravat : dur�e de vie infinie, d�chets D : d�chets D du b�timent qui s'est d�truit, co�t en ressources D identique � celui du b�timent d�truit

- Fond du menu principal : chargement d'une partie sauvegard�e (malgr� un + long tps de chargement) : lecture de l'animation de cette partie
*/

/*
Liens utiles :
- Description compl�te du procussus d'�clairage 3D temps r�el et shaders associ�s : http://pisa.ucsd.edu/cse125/2006/Papers/Lighting-Per_Pixel_Shading_and_Optimizations.pdf
- Calcul du brouillard en GLSL : http://www.ozone3d.net/tutorials/glsl_fog/index.php
- Description des shaders GLSL : http://www.irit.fr/~Mathias.Paulin/IUP-SI/M2/4-glsl.pdf
- Description des shaders en HLSL : http://mathinfo.univ-reims.fr/image/dxShader/cours/08-HLSL.pdf
- Fichier de ressources d�taill� pour les propri�t�s de l'application : http://msdn.microsoft.com/en-us/library/aa381058%28VS.85%29.aspx
- Caract�res pour les noms de fichiers interdits dans les syst�mes d'exploitation : http://en.wikipedia.org/wiki/Filename et http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247.aspx
- Librairies automatiquement ajout�es par le compilateur de Visual C++ et conflits possibles entre-elles : http://msdn.microsoft.com/en-us/library/aa267384.aspx
- Chargement diff�r� des DLLs : http://msdn.microsoft.com/fr-fr/library/151kt790.aspx ; http://www.codeproject.com/KB/DLL/Delay_Loading_Dll.aspx ; http://www.pcreview.co.uk/forums/delayload-pragma-fixed-whidbey-t1430386.html ; et : http://msdn.microsoft.com/en-us/library/7f0aews7.aspx
- Exceptions SEH de MSVC : http://members.gamedev.net/sicrane/articles/exception.html
- Conseils d'optimisation pour DirectX : http://msdn.microsoft.com/en-us/library/windows/desktop/bb147263.aspx
- Optimisation avanc�s des shaders HLSL : http://developer.amd.com/media/gpu_assets/Dark_Secrets_of_shader_Dev-Mojo.pdf
*/

/*
Nouveaut�s (Pour le ??/??/12 : Version 0.0.0.1)
- Ajout de l'option du facteur de visibilit� des temps (weatherVisibilityFactor) dans la configuration du jeu, pour permettre une meilleure visibilit� du jeu lors des temps sombres
- R�organisation compl�te du menu inf�rieur de construction de la GUI du jeu et ajout du mode r�actif � la descente/mont�e de ce menu : ce menu s'affiche lorsque la souris est sur lui, puis se masque lentement lorsque la souris quitte ce menu
- Nombreuses modifications et am�liorations de la version enfant du jeu :
	- R�int�gration des usines : elles ne consomment simplement plus les ressources non productibles par le joueur, mais continuent de produire leur ressource d�di�e : permet la r�int�gration des ressources verre, tuiles et ciment.
	- R�int�gration du papier : c'est maintenant une ressources consomm�e en grande quantit� par les habitants pour leur satisfaction, ce qui ajoute un peu de difficult� � cette version du jeu et un int�r�t � la satisfaction des habitants.
	- Pour la production quotidienne de bois des arbres : Les arbres produisent maintenant du bois proportionnellement � leur �ge (jusqu'� 1 an : 5 kg/arbre/J).



Pour le 27/08/12 : Version 0.0.0.1 : Aucune r�trocompatibilit� avec les sauvegardes cr��es dans des les versions ant�rieures du jeu !
- Cr�ation d'une version "Enfant" du jeu (Kid version) : Suppression compl�te des ressources non productibles : Cette version est dor�navant compl�tement jouable (bien que tr�s simplifi�e) !
- Ajout des derni�res textures de Normal Mapping
- Suppression du type de terrain Paged car bugg� et non adapt� � ce type de jeu
- Ajout des effets post-rendu et des param�tres correspondants dans le menu options. Ces effets sont compl�tement fonctionnels, seul probl�me mineur : l'anticr�nelage doit �tre d�sactiv� pour pouvoir les utiliser, sinon cela provoque des incoh�rences de Z-Buffer.
- Mode multijoueur r�activ� : fonctionnel mais non stable sur de longues parties, d� � des probl�mes de synchronisation
- Nouvelles r�gles : Evite que le joueur ne gagne de l'argent � n'avoir que des maisons sans leur fournir aucun confort (ni �lectricit�, ni eau, ni aucune ressource pour leur satisfaction)
	- Les imp�ts sont maintenant d�pendants du nombre de personnes dans les habitations
	- Les vitesses d'emm�nagement et de d�m�nagement des habitants sont maintenant aussi d�pendantes de leur satisfaction r�elle (plus ils sont satisfaits, plus ils emm�nagent vite, et inversement pour leur d�m�nagement).
		En-dessous de 20% de satisfaction r�elle, les habitants d�m�nagent des maisons. Le seuil de satisfaction (20% par d�faut) est d�pendant de la difficult� du jeu (de 10% en mode Facile � 40% en mode Difficile actuellement).
	- Chaque habitation a un minimum de 1 habitant, pour �viter que la population du monde atteigne 0, et donc pour pallier au probl�me suivant :
		Sans cette limite, lorsque la satisfaction de la population est faible, la population du monde diminue progressivement jusqu'� atteindre 0 habitants.
		Une fois que la population du monde est de 0 habitants, la satisfaction de la population passe instantan�ment � 100 %.
		La population du monde se remet alors � augmenter � sa vitesse maximale. Une fois que la population du monde n'est plus nulle, la satisfaction de la population rechute � sa faible valeur,
		et la population du monde diminue encore jusqu'� 0 habitants, et ainsi de suite. Ainsi, � des taux faibles de satisfaction il se cr�e une oscillation de la population entre 0 habitants et 1 habitant,
		et la satisfaction fait des sauts brusque de sa faible valeur � 100 %, ce qui rend la population du monde instable.
	- Le pourcentage d'�nergie disponible pour les habitations influe aussi sur le pourcentage de satisfaction des habitants (facteur entre *0.5 � 0% d'�nergie � *1 � 100% d'�nergie)
- Refonte majeure du syst�me de mise � jour du monde et des b�timents :
	- Il ne devrait plus y avoir de bugs aux calculs des poucentages de ressources disponibles pour les b�timents, et les factures d'�lectricit� et d'eau sont maintenant correctement calcul�es et affich�es
- D�blocage des b�timents suivant la population : il est dor�navant impossible de construire un Grand building individuel d�s le d�but du jeu par exemple, il faut atteindre un certain seuil de population pour le d�bloquer.
- Les fum�es des b�timents ne sont plus affich�es lorsque le facteur de production de ces b�timents est nul
- Pour les b�timents non textur�s (de couleur compl�tement grise dans les versions pr�c�dentes), des mat�riaux color�s leur ont �t� associ�s + Fin de l'architecture la pompe � eau (avec l'animation correspondante)
- Durant le chargement d'une partie sauvegard�e, le manque d'un bloc XML (ex : <GUI> ... </GUI>) ou sa mauvaise position dans le fichier n'emp�che plus le chargement de la partie
	(Attention : le support du mauvais ordre des blocs <Batiment> ... </Batiment> n'a pas �t� r�solu (voir EcoWorldSystem.cpp : EcoWorldSystem::load, ligne 262 environ))
- Enregistrement du nom des b�timents dans les sauvegardes au lieu de leur ID (permet un changement des ID des b�timents sans automatiquement invalider toutes les sauvegardes d�j� cr��es jusqu'� pr�sent)
	et changement du nom de base des blocs des b�timents de <Batiment123> en <Batiment> car ces nombres ralentissaient l�g�rement le chargement et n'�taient pas n�cessaires
- Un arbre produit maintenant du bois � sa destruction proportionnellement � son �ge (jusqu'� 1 an)

Pour le 03/05/12 : Version 0.0.0.0 :
- Meilleure gestion des factures d'eau et d'�lectricit� dans les b�timents, ajout de la gestion de la consommation d'�lectricit�
- Refonte compl�te du menu options : mieux organis�, plus d'options, ajout d'une fen�tre de confirmation des param�tres en cas de modification de param�tres importants
- Meilleure transitions des temps, par l'utilisation d'un shader permettant l'interpolation entre 2 textures de ciel, et par la r��criture compl�te de la classe WeatherManager, qui fournit maintenant une gestion du temps plus claire et plus optimis�e
- Support am�lior� du Normal Mapping (r�solution des bugs, seules quelques normal maps restent � cr�er)
- Animation des arbres par rotation (comme pouss�s par le vent)
- Ajout du module d'animation de la cam�ra suivant une ligne personnalisable par l'utilisateur (au moyen d'une fen�tre graphique) et de la possibilit� d'enregistrer/charger cette animation depuis un fichier de sauvegarde
- Optimisation de la cam�ra RTS et mise � jour de ses animators et de ses enfants (dont la cam�ra FPS fait maintenant partie) avec le temps r�ellement �coul�, ind�pendament de la vitesse du jeu, et permettant aussi de d�placer la cam�ra lorsque le jeu est en pause
- Ajout des packs de musiques personnalisables, int�gration des fichiers d'artiste (titre, logo, ic�ne) de Romain DARCEL, et ajout d'un curseur de souris sp�cifique au jeu
- Terrain "Terres accueillantes.ewt" ajout� : Correspond aux instructions de terrains : Colline + Rivi�re + Mer + Zone de construction plane
- Lien dynamique de la DLL d'IrrKlang (irrKlang.dll) : Permet dor�navant de d�marrer le jeu sans sa pr�sence
- Passage � "Irrlicht SVN 1.8.0-alpha (r�vision 4020) Modified for EcoWorld"
- Possibilit� de continuer la partie actuelle une fois gagn�e
- Am�lioration et optimisation de tous les shaders
- Scroll bar de s�lection de la vitesse du jeu am�lior�e (+ Impl�mentation du mode Boost)
- Affichage d'une v�ritable date (ex : "25 juin 2003") pour le temps �coul� actuel en haut � droite de la GUI du jeu (le premier jour est "1 janvier 2000")
	+ R�affichage du temps �coul� sous l'ancien format dans son texte de description

Pour le 09/11/11 :
- Objectifs
- Blocs de b�ton sous les b�timents
- Textes de description des terrains + Cr�ateur de terrain
- Enregistrement/Chargement des positions des fen�tres de l'interface du jeu
- D�but de support du Normal Mapping (shader bugg� actuellement, mais architecture globale termin�e)
*/

/*
A voir avec Thomas :

- Revoir les valeurs des modifiers de difficult� pour �quilibrer le jeu m�me � des difficult�s inf�rieures



Anciennes questions :

- Production d'�lectricit� : impl�menter des Barrages hydro-�lectriques ? (grande production d'�lectricit�)
	=> A oublier

- Remise � l'�chelle des b�timents :
	- Maisons et immeubles : Taille de base disis�e par 3
		-> Probl�me pour le Chalet (taille non enti�re)
			=> Arrondir au-dessus
	- Buildings : Taille de base divis�e par 2 seulement (pour obtenir des tailles enti�res) -> Contradiction avec la mise � l'�chelle des maisons (mais �chelle globale correcte)
		=> Valide

- Revoir les nouvelles couleurs des b�timents non textur�s

- Revoir les populations minimales n�cessaires pour d�bloquer les b�timents : StaticBatimentInfos::unlockPopulationNeeded
	+ D�sactiver (voire masquer ?) les boutons de la GUI pour les b�timents bloqu�s ?

- Echelle des b�timents revue :
	- Taille des panneaux solaires un peu grande (compar� avec la maison avec panneaux solaires), mais ils ont d�j� la taille minimale !
		-> Peu important
	- Certaines maisons devraient avoir une taille non enti�re (d� � leur redimendionnement) : elles ont actuellement la taille sup�rieure, mais leur bloc de b�ton est ainsi plus grand !
	=> Fait

- N�cessit� de la valeur r�elle de chaque ressource pour impl�menter la vente et l'achat de ressources
	-> Pifom�tre

- Note : R�f�rences :
	- M6 (journal t�l�vis�) : 390 kg de d�chets par habitant par an en moyenne
		-> Bonnes valeurs des d�chets actuelles (1 kg / jour / habitant)
	- Professeur de G�ographie : 85 kg de viande consomm�es par habitant par an en moyenne (en France) (=> 233 g / jour / habitant)
		-> Mauvaise valeurs actuelles (1 kg / jour / habitant)

- Affichage de la v�riable date :
	- Afficher "1er janvier" au lieu de "1 janvier" ?
	- Les mois comportent tous 30 jours : Probl�mes avec les 29/30 f�vrier et les mois de 31 jours !
	- Donne au jeu une dimension r�elle : ne concorde pas tellement avec l'id�e d'un monde (compl�tement vierge au d�part) � construire
	- Les mois ne concordent pas avec le temps actuel (ex : il peu faire parfaitement beau en d�cembre !) : G�nant ?
		=> Peu important

- Modifiers : Prix des b�timents modifi�s -> Temps de construction/destruction modifi�s aussi en cons�quence ?
	-> Non

- R�cup�ration des eaux de pluie ? (depuis que la pluie a �t� impl�ment�e)
	-> Non : la pluie permet d'alimenter les ruisseaux

- Cr�er une liste des raccourcis claviers, des fonctionnalit�s du jeu, de ses options, de ses particularit�s et les consigner dans un Manuel Utilisateur

- Animer le fond du menu principal avec une "vid�o" (images anim�es logiciellement) :
	=> Avantages : chargement plus rapide (pas de cr�ation de terrain, seulement de textures), pas besoin de charger de b�timents en avance, apparence professionnelle...
	-> Demander des propositions d'id�es pour cette animation



- Que se passe-t-il lorsqu'un b�timent a atteint la fin de sa dur�e de vie ?
	-> Il est d�truit, mais quel effet cela a-t-il sur le budget, l'ES, les d�chets... ?
- Que se passe-t-il pendant la construction d'un b�timent (� part attendre qu'il soit construit, y a-t-il de la pollution rejet�e) ?
	-> Pendant la construction d'un b�timent, les ressources n�cessaires sont-elles pr�lev�es une fois pour toutes au d�but de la construction, ou petit � petit durant la construction ?
- Que se passe-t-il pendant la destruction d'un b�timent (continue-t-il � rejeter des d�chets et de l'ES, consommer des ressources) ?
	-> Pendant la destruction d'un b�timent, les ressources n�cessaires sont-elles pr�lev�es une fois pour toutes au d�but de la destruction, ou petit � petit durant la destruction ?
- Repr�sentation 3D de la destruction identique � celle de la construction !?
- effetSerreC et dechetsC sont rejet�s � la fin de la construction du b�timent ou lorsque l'ordre de construction est lanc� ?
- effetSerreD et dechetsD sont rejet�s � la fin de la destruction du b�timent ou lorsque l'ordre de destruction est lanc� ?
	-> Actuellement : au lancement des ordres

- Peut-on annuler la construction/destruction d'un b�timent ? Quelles cons�quences ?
- Que se passe-t-il lorsqu'on d�truit un b�timent pendant sa construction ?


- Quelles informations afficher dans le tableau r�capitulatif des fins de mois ?
- R�organiser l'ordre des informations dans la fen�tre d'informations et dans le tableau r�capitulatif des fins de mois

- Doit-on afficher les ressources consomm�es par les habitants de la maison s�lectionn�e dans la fen�tre d'informations ?
	-> Actuellement : Non
- Doit-on aussi mettre � jour le loyer r�el des maisons et la production r�elle des usines (affich�e dans la fen�tre d'informations) avec l'�nergie disponible et la satisfaction des habitants ?
	-> Actuellement : Oui
- Dans la fen�tre d'informations, remplacer "Loyer" par "Loyer moyen", puisque d�pendant d'autres facteurs externes (�nergie, satisfaction)
- Revoir l'organisation des textes dans la fen�tre d'informations
- Dans la fen�tre d'informations, passer les "D�chets (J)" en "D�chets (M)" pour �viter la confusion avec l'"Evolution des d�chets (M)" ? (idem pour effet de serre)

- Eviter la s�lection des pans de route ?
- Renommer les onglets de la construction -> R�organiser tous les boutons de construction
- Revoir les textes dans la fen�tre d'informations sur la destruction d'un b�timent
- Organisation de l'�cran "Perdu" � revoir


- Cr�er les fonctions pour les calculs des informations sur les b�timents (� placer dans [Static]BatimentInfos::calculateFunctionsData())

- Recalculer les prix des maisons | immeubles | buildings individuel(les) / BC / avec PS / grand(e)s
- Feuilles de calcul pour les d�chets, ES, ressources, prix... (une est d�j� propos�e)
- Augmenter les factures d'�lectricit� des maisons (presque *3 !) -> Revoir les consommations d'eau et d'�lectricit� ainsi que leurs factures
- Impl�mentation des derniers b�timents indispensables : rev�rifier les valeurs
	Donn�es des nouveaux b�timents � revoir !

- Revoir les �oliennes : http://fr.wikipedia.org/wiki/%C3%89olienne#Poids_.C3.A9conomique_des_acteurs_de_l.27industrie_.C3.A9olienne
	-> Plus grandes pr�cisions ici : http://fr.wikipedia.org/wiki/%C3%89nergie_%C3%A9olienne
		et ici : http://eolienne.f4jr.org/
- En r�alit�, hydroliennes s�rement beaucoup plus ch�res : http://fr.wikipedia.org/wiki/Hydrolienne
- Petites + Grosses hydroliennes ? (http://generationsfutures.chez-alice.fr/energie/hydrolienne.htm)
	-> Dans l'embouchure des fleuves / Dans la mer
- Hydroliennes compl�tement immerg�es ou d�passant � la surface ?
- Rentabilit� des hydroliennes : 1 � / W
	-> R�alit� : 2 turbines de 18m ! <=> 1 MW /h (http://generationsfutures.chez-alice.fr/energie/hydrolienne_illustration_ouestfrance.jpg)



D�cid� :

- (Usine de plastique (ma�s + eau))

- Ponts (500 � par m�) en pierre

- Passer des Watts aux MegaWatts/KiloWatts (plus r�aliste et plus facile � manipuler) ? -> 4 kW par maison

- Achat de terrains (1km * 1km) -> 1 000 000 �
- Achat possible de 36 terrains
- Terrain de d�part : 2km * 2km

- Stockage des ressources -> Cr�er un entrep�t

- Personnes travaillant -> Pour le moment : personne ne travaille !!!

- Maisons de d�part

- Des gisements !!!

- ex : arbres -> bois (scierie)
gisement de fer -> fer (m�tallerie)
gisement de pierre -> pierre (fabrique de pierre)
gisement de sable -> sable (sabli�re)

- (((Recyclage ; Commerce entre villes ; Saisons (production diff�rente))))

-> Tutoriel ! ; Sc�narios ! ; Campagne ?

- Ce qui se produit lorsque les citoyens n'ont pas assez d'une ressource pour vivre (comme par exemple, l'eau) -> Les gens partent et ne payent plus
- Est-ce qu'on peut acheter ou vendre des ressources ? -> Oui
- Est-ce que les habitants consomment des mat�riaux de construction (pour r�nover leur maison, se cr�er une petite cabane en bois, ...) ? -> Oui, apr�s ils payent plus

- Le reste des ressources et des b�timents, et les r�gles avanc�es du jeu (comme la recherche ou le recyclage)
- Centres commerciaux, Hopitals... -> Services municipaux et d'accueil aux personnes ?



Liste des b�timents :
---------------------
- Maisons, immeubles, buildings, ch�let : on garde ce qui reste actuellement

- Production des ressources (Usines) :
	-> Tant que les gisements n'auront pas �t� impl�ment�s, les productions de base (ex : pierre, sable, calcaire...) pourront produire n'importe o� sur la carte
{
	- Eau :
		- Extraction depuis les nappes phr�atiques : Pompe d'extraction de l'eau (production de base) (d�bloqu�)
		- Traitement de l'eau des ruisseaux/mers : Usine de traitement de l'eau (plac�e � c�t� d'un point d'eau) (production de base) (bloqu� au d�part)
	- Verre :
		- Usine de verre
	- Pierre :
		- Fabrique de pierre (production de base)
	- M�tal (anciennement : fer) :
		- M�tallerie
	- Bois :
		- Scierie (pas besoin d'arbre autour pour le moment) (production de base)
	- Ciment :
		- Usine de ciment
	- Tuiles :
		- Usine de tuiles
	- Sable :
		- Sabli�re (production de base)
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
		- Port de p�che (plac�e � c�t� d'un point d'eau) (production de base)
		- Pisciculture (production de base) (cr�ation d'un point d'eau pour l'occasion !)
	- Pain :
		- Boulangerie
		- Usine de pain
	- Huile :
		- Huilerie (n�cessite du tournesol)	<- Nom officiel � revoir !
	- Vin :
		- Pressoir
	- Lait et Vaches :
		- Ferme de vache (n�cessite de l'avoine)	<- Nom officiel � revoir !
	- Laine et Moutons :
		- Ferme de moutons (n�cessite de l'avoine)	<- Nom officiel � revoir !
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
	- Bl� :
		- Champ de bl� (pas besoin de plants de bl� autour pour le moment) (production de base)
	- Tournesol :
		- Champ de tournesol (pas besoin de plants de tournesol autour pour le moment) (production de base)
	- Avoine :
		- Champ d'avoine (pas besoin de plants d'avoine autour pour le moment) (production de base)

	- V�tements :
		- Usine de textile (n�cessite de l'indigo et de la laine)
	- Indigo :
		- Champ d'indigo (pas besoin de plants d'indigo (indigotier) autour pour le moment) (production de base)
}



Usines de recyclage :
- Ne recycle que ce qui ressort des usines et des habitations
	-> Les usines et les habitations rejettent des d�chets recyclables
	-> Une usine de recyclage peut acceuillir un certain nombre de d�chets recyclables, et les d�chets recyclables qui ne peuvent pas �tre trait�s sont perdus en d�chets

- (Centrale nucl�aire ?	<- N�cessite des ressources et d�chets nucl�aires (� impl�menter) !)



Termin� : (peut-�tre encore � revoir)
---------
- R�server les imp�ts aux habitations (seuls b�timents utilisant ce param�tre) ?

- Afficher une v�ritable date (ex : "25 juin 2003") pour le temps �coul� actuel en haut � droite de la GUI du jeu ? (le premier jour serait "1 janvier 2000")

- Enregistrer les positions des fen�tres de la GUI dans les sauvegardes + Cr�er une touche pour r�initialiser la GUI du jeu (touche "C")

- Pouvoir arr�ter les usines

- A partir de combien de dettes le joueur a-t-il perdu ? -> -100 000 � <- A revoir

- Dur�e de vie des b�timents : (dont 20% al�atoire)
	Maison/Chalet 100 ans
	Immeuble 95 ans
	Buildings 90 ans
	Usine d'incin�ration des d�chets 50 ans
	Usine (petite) 40 ans
	Usine (grande) 60 ans
	Centrale � charbon 30 ans
	Panneau solaire 10 ans
	Eolienne 70 ans
	Hydrolienne 60 ans
	D�charge 200 ans
	Scierie/Fonderie/Taileur/... 40 ans
	Arbre 150 ans (al�atoire : 40 %)

- Tps de construction/destruction
	- Co�t de construction : prixC / 2
		-> 5000 �/ mois peuvent �tre construits (5 personnes � 1000 � par mois)
		==> 10 000 � / C <-> 1 mois de construction
		ex : maison individuelle : 60 000 � -> 6 mois de construction
	-> Destruction : tps de construction/10 ==> 10 000 � / D <-> 1 mois de destruction

- Eau : Mat�riau ou Nourriture ? -> Mat�riau !

- Fonctionnement actuel des loyers : � la fin de chaque mois, le loyer est per�u, que la maison existe depuis 20 jours ou qu'elle vienne juste d'�tre construite
	-> On laisse comme c'est

- Les ressources de d�part et l'or de d�part

- Que se passe-t-il lorsque l'�nergie tombe en-dessous de 0 ? (par exemple : en cr�ant des batiments de production d'�nergie, en cr�ant des batiments utilisant cette �nergie, puis enfin en d�truisant les batiments de production d'�nergie)
	-> Peut-on d�truire un batiment si on n'aura ensuite plus assez d'�nergie ? Oui, mais le malus sera ailleurs

D�charges :
- Une d�charge ne co�te pas cher, prend beaucoup de place et peut contenir beaucoup de d�chets
- Les d�chets disparaissent tr�s lentement
- On peut recharger une d�charge, jusqu'� un seuil maximum
- En d�truisant une d�charge, les d�chets qu'elle contient reviennent dans le compteur de d�chets, si elle n'en contient plus, la destruction d'une d�charge ne co�te qu'un peu d'argent

Usines d'incin�ration des d�chets :
- Les d�chets entrants deviennent de l'ES (voir combien de d�chets font combien d'ES)

Usines de production d'�nergie :
- Eoliennes
- Champs de panneaux solaires -> Fait
- Centrales � charbon (de bois) (bois en entr�e)
- Hydroliennes


- Des bonus de jeu suivant une difficult� choisie, permettant d'avoir plus d'argent et moins d'effet de serre et de d�chets en mode Facile, et plus de taxes en mode Difficile
- Que se passe-t-il quand le joueur a beaucoup trop d'ES ? -> Taxe quadratique : 50k -> 1M
- Que se passe-t-il quand le joueur a beaucoup trop de d�chets ? -> Taxe quadratique : 100k -> 2M



******************** Batiments de production d'�nergie ********************

- Centrale � charbon :
Prix : 120000 (C) / 12000 (D)

5W <-> 1kg de bois / M <-> 0.08 � / M
300 000 W max => 60 000 kg de bois /M max !

- 1 Panneau solaire : 1W <-> 2.5 �
Prix : 1250 �
Production : 500 W

- 1 Eolienne : 1W <-> 2 �
Prix : 80000 �
Production : 40000 W de base (OU : 20000 W par 100 m au dessus du niveau de la mer avec 50 000 W max -> si possible !, � revoir)

- 1 Hydrolienne : 1W <-> 1.5 �
Prix : 90000 �
Production : 60000 W
*/

/*
Note sur les correspondances des tailles :
	2.0f sous Blender <=> 1.0f sous EcoWorldSystem <=> 40.0f (TAILLE_OBJETS) sous Irrlicht
*/

/*
M�thode pour ajouter un batiment :
----------------------------------
- Ajouter son ID dans Batiments.h -> BatimentID (attention : la position de l'ID dans l'�num�ration d�termine aussi l'ordre de mise � jour du b�timent dans sa liste)
- Ajouter ses caract�ristiques dans Batiments.h -> StaticBatimentInfos::setID (toutes les caract�ristiques ont des valeurs par d�faut, certaines peuvent donc �tre omises) (ne pas oublier d'indiquer son nom !)
	/!\ : Si c'est une habitation, bien renseigner dans le d�but de la fonction calculateFunctionsData() la variable isHabitation !
	/!\ : Si c'est un arbre, bien renseigner dans le d�but de la fonction calculateFunctionsData() la variable isTree !
- D�terminer si ce b�timent :
	- autorise le r�glage de son pourcentage de production par l'utilisateur, en renseignant la m�thode StaticBatimentInfos::needPourcentageProduction (facultatif : non par d�faut)
	- n�cessite un bloc de b�ton sous lui, en renseignant la m�thode StaticBatimentInfos::needConcreteUnderBatiment (facultatif : oui par d�faut)
	- peut �tre cr�� par rectangle de s�lection, en renseignant la m�thode StaticBatimentInfos::canCreateBySelectionRect (facultatif : non par d�faut)
- Indiquer sa liste pour la mise � jour du monde dans EcoWorldSystem::addBatimentToLists
- Mettre � jour son comportement sp�cial face au monde dans Batiment::updateWorldFromBatimentID() (facultatif)
- Ajouter son bouton dans la GUI dans GUIManager.cpp -> GUIManager::createGameGUI (en utilisant la fonction GUIManager::addBoutonBatiment)
- Ajouter son mod�le 3D dans EcoWorldRenderer.cpp -> EcoWorldRenderer::loadBatiment
- Ajouter son texte dans EcoWorldRenderer.cpp -> EcoWorldRenderer::loadTextBatiment
*/

/*
M�thode pour ajouter une ressource :
------------------------------------
- Ajouter son ID dans Ressources.h -> Ressources::RessourceID
- Ajouter son nom complet dans Ressources.h -> Ressources::getRessourceName et dans Ressources.h -> Ressources::getRessourceNameChar (les noms doivent �tre dans le m�me ordre que les ID)
- Ajouter son unit� dans Ressources.h -> Ressources::getRessourceUnit (les unit�s doivent �tre dans le m�me ordre que les ID)
- Ajouter les co�ts et les productions de cette ressource pour les batiments dans Batiments.h -> BatimentInfos::setID (facultatif)
- Ajouter la consommation de cette ressource par jour et par personne dans EcoWorldInfos.h -> update (facultatif)
*/

/*
M�thode pour ajouter un temps (weather) :
-----------------------------------------
- Ajouter son ID dans Weathers.h -> WeatherID
- Ajouter ses caract�ristiques dans Weathers.h -> WeatherInfos::setID (toutes les caract�ristiques ont des valeurs par d�faut, certaines peuvent donc �tre omises)
- Cr�er son interpolation dans WeatherManager.cpp -> WeatherManager::calculateCurrentWeatherInfos en utilisant la macro INTERPOLATE si nec�ssaire
- Cr�er ses effets sur le device dans EcoWorldRenderer.cpp -> EcoWorldRenderer::updateWorldFromWeatherManager() (facultatif)
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

En effet, le second argument demand� par swprintf_s n'est en fait pas la taille m�moire du buffer en octets (comme on pourrait le croire � cause de son type : size_t),
mais en fait sa taille en nombre de caract�res (soit deux fois moins si on consid�re que sizeof(wchar_t) = 2 octets) !

En mode DEBUG, cette diff�rence peut occasionner des erreurs d'�criture en-dehors d'une zone m�moire allou�e (bufer overflow),
car swprintf_s semble effacer compl�tement le buffer fourni. Cette erreur se traduit par erreur d'ex�cution � la fin de la fonction (accolade fermante finale) utilisant swprintf_s :
Run-Time Check Failure #2 - Stack around the variable 'str' was corrupted.

En mode RELEASE, le d�passement de capacit� ne semble pas avoir �t� av�r� (erreur non d�tect�e (silencieuse ?), ou peut-�tre que contrairement � la version DEBUG, swprintf_s n'efface pas le buffer),
mais il reste toujours un bug possible, qui appara�t si plus de la moiti� du buffer fourni est �crite.

*/

#endif
