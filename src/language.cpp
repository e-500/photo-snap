/* Photo Snap is licensed under the GNU General Public License, version 3 or later. */
#include "language.h"

#include <QHash>
#include <QLocale>

namespace {
using Catalog = QHash<QString, QString>;

const QHash<QString, Catalog> &catalogs()
{
    static const QHash<QString, Catalog> value = {
        {QStringLiteral("IT"), {
            {"&File", "&File"}, {"&Open…", "&Apri…"}, {"&Quit", "&Esci"}, {"&View", "&Visualizza"},
            {"&Navigate", "&Navigazione"}, {"Open a photo to get started · Ctrl+O", "Apri una foto per iniziare · Ctrl+O"},
            {"Previous photo", "Foto precedente"}, {"Next photo", "Foto successiva"}, {"Zoom in", "Ingrandisci"},
            {"Zoom out", "Riduci"}, {"Fit to window", "Adatta alla finestra"}, {"Actual size", "Dimensioni reali"},
            {"Rotate", "Ruota"}, {"Copy", "Copia"}, {"Full screen", "Schermo intero"}, {"Exit full screen", "Esci dallo schermo intero"},
            {"Library", "Raccolta"}, {"Photo tools", "Strumenti foto"}, {"Open", "Apri"}, {"Open photo", "Apri foto"},
            {"Open library", "Apri raccolta"}, {"Edit metadata", "Modifica metadati"}, {"Images (%1);;All files (*)", "Immagini (%1);;Tutti i file (*)"},
            {"Unable to open photo", "Impossibile aprire la foto"}, {"Could not read “%1”.\n%2", "Impossibile leggere “%1”.\n%2"},
            {"Image cache unavailable", "Cache immagini non disponibile"}, {"Photo copied to clipboard", "Foto copiata negli appunti"},
            {"%1 selected photo(s) copied to clipboard", "%1 foto selezionate copiate negli appunti"},
            {"Edit selected photo metadata?", "Modificare i metadati delle foto selezionate?"},
            {"Apply the same shooting date, author, and copyright to all %n selected photos?", "Applicare la stessa data di scatto, lo stesso autore e copyright a tutte le %n foto selezionate?"},
            {"Update", "Aggiorna"}, {"When unchecked, keep each photo's existing value.", "Se deselezionato, mantiene il valore esistente di ogni foto."},
            {"Shot date/time", "Data/ora di scatto"}, {"Photographer", "Fotografo"}, {"Author", "Autore"}, {"Copyright", "Copyright"},
            {"Some library entries could not be updated", "Impossibile aggiornare alcune voci della raccolta"}, {"Some photos could not be updated", "Impossibile aggiornare alcune foto"},
            {"By shooting date", "Per data di scatto"}, {"By creation date", "Per data di creazione"}, {"Scan folder", "Analizza cartella"},
            {"Close cached photos", "Chiudi foto memorizzate"}, {"No cached photos yet", "Nessuna foto in cache"}, {"No photos in this year", "Nessuna foto in questo anno"},
            {"Photo scan log", "Registro scansione foto"}, {"Pause", "Pausa"}, {"Resume", "Riprendi"}, {"Stop", "Interrompi"},
            {"Choose a folder to scan", "Scegli una cartella da analizzare"}, {"Restore hidden cached photos?", "Ripristinare le foto nascoste nella cache?"},
            {"If a cached photo is still present in this folder, should it be made visible in the cache browser?", "Se una foto in cache è ancora presente in questa cartella, renderla visibile nel browser della cache?"},
            {"Scanner not found", "Scanner non trovato"}, {"Could not find the scanner application:\n%1", "Impossibile trovare l'applicazione scanner:\n%1"},
            {"Scanning…", "Scansione…"}, {"Scanning folder: %1", "Analisi cartella: %1"}, {"Photo moved", "Foto spostata"},
            {"This photo was found at a new path.\n\nFrom: %1\nTo: %2\n\nRelink this photo and apply the folder change to other matching entries?", "Questa foto è stata trovata in un nuovo percorso.\n\nDa: %1\nA: %2\n\nRicollegare la foto e applicare la modifica della cartella alle altre voci corrispondenti?"},
            {"Relink approved: %1 → %2", "Ricollegamento approvato: %1 → %2"}, {"Relink declined; aborting scan.", "Ricollegamento rifiutato; scansione interrotta."},
            {"Select every photo in this year", "Seleziona tutte le foto di questo anno"}, {"Select every photo in this month", "Seleziona tutte le foto di questo mese"}, {"Undated", "Senza data"},
            {"Could not hide some photos", "Impossibile nascondere alcune foto"}, {"… and %1 more", "… e altre %1"},
            {"Delete photos permanently?", "Eliminare definitivamente le foto?"},
            {"This will permanently delete %n selected image file(s):\n\n%1\n\nContinue?", "Questo eliminerà definitivamente %n file immagine selezionati:\n\n%1\n\nContinuare?"},
            {"Could not delete file: %1", "Impossibile eliminare il file: %1"}, {"Could not remove cache entry for %1: %2", "Impossibile rimuovere la voce dalla cache per %1: %2"},
            {"Some photos could not be deleted", "Impossibile eliminare alcune foto"}, {"Settings", "Impostazioni"},
            {"Database file: %1", "File database: %1"}, {"Reveal database in file explorer", "Mostra il database nella cartella"},
            {"Could not open folder", "Impossibile aprire la cartella"}, {"Could not open %1", "Impossibile aprire %1"}, {"Language", "Lingua"},
            {"%1: %2", "%1: %2"}, {"%1: metadata saved, but hash could not be refreshed (%2)", "%1: metadati salvati, ma impossibile aggiornare l'hash (%2)"},
            {"%1: metadata saved, but the library entry could not be updated (%2)", "%1: metadati salvati, ma impossibile aggiornare la voce della raccolta (%2)"},
            {"Stop requested; waiting for the current image to finish…", "Interruzione richiesta; attendo il completamento dell'immagine corrente…"},
            {"Scanner exited with code %1.", "Lo scanner è terminato con codice %1."}, {"Could not start scanner: %1", "Impossibile avviare lo scanner: %1"},
            {"Photo scan log — 0.0 %", "Registro scansione foto — 0,0 %"}, {"Photo scan log — %1 %", "Registro scansione foto — %1 %"},
            {"This photo was found at a new path.\n\nFrom: %1\nTo: %2\n\nRelink this photo and use this folder change for other matching entries? Choosing No aborts this scan.", "Questa foto è stata trovata in un nuovo percorso.\n\nDa: %1\nA: %2\n\nRicollegare la foto e applicare la modifica della cartella alle altre voci corrispondenti? Scegliendo No la scansione verrà interrotta."},
            {"Restart Photo Snap to apply the language change.", "Riavvia Photo Snap per applicare la modifica della lingua."}, {"Language changed", "Lingua modificata"}
        }},
        {QStringLiteral("DE"), {
            {"&File", "&Datei"}, {"&Open…", "&Öffnen…"}, {"&Quit", "&Beenden"}, {"&View", "&Ansicht"}, {"&Navigate", "&Navigation"},
            {"Open a photo to get started · Ctrl+O", "Öffne ein Foto, um loszulegen · Strg+O"}, {"Previous photo", "Vorheriges Foto"}, {"Next photo", "Nächstes Foto"},
            {"Zoom in", "Vergrößern"}, {"Zoom out", "Verkleinern"}, {"Fit to window", "An Fenster anpassen"}, {"Actual size", "Originalgröße"},
            {"Rotate", "Drehen"}, {"Copy", "Kopieren"}, {"Full screen", "Vollbild"}, {"Exit full screen", "Vollbild beenden"}, {"Library", "Bibliothek"},
            {"Photo tools", "Fotowerkzeuge"}, {"Open", "Öffnen"}, {"Open photo", "Foto öffnen"}, {"Open library", "Bibliothek öffnen"}, {"Edit metadata", "Metadaten bearbeiten"},
            {"Images (%1);;All files (*)", "Bilder (%1);;Alle Dateien (*)"}, {"Unable to open photo", "Foto kann nicht geöffnet werden"}, {"Could not read “%1”.\n%2", "„%1“ konnte nicht gelesen werden.\n%2"},
            {"Image cache unavailable", "Bildcache nicht verfügbar"}, {"Photo copied to clipboard", "Foto in die Zwischenablage kopiert"}, {"%1 selected photo(s) copied to clipboard", "%1 ausgewählte(s) Foto(s) in die Zwischenablage kopiert"},
            {"Edit selected photo metadata?", "Metadaten der ausgewählten Fotos bearbeiten?"}, {"Apply the same shooting date, author, and copyright to all %n selected photos?", "Dasselbe Aufnahmedatum, denselben Autor und Copyright auf alle %n ausgewählten Fotos anwenden?"},
            {"Update", "Aktualisieren"}, {"When unchecked, keep each photo's existing value.", "Wenn deaktiviert, den vorhandenen Wert jedes Fotos beibehalten."}, {"Shot date/time", "Aufnahmedatum/-zeit"},
            {"Photographer", "Fotograf"}, {"Author", "Autor"}, {"Copyright", "Copyright"}, {"Some library entries could not be updated", "Einige Bibliothekseinträge konnten nicht aktualisiert werden"}, {"Some photos could not be updated", "Einige Fotos konnten nicht aktualisiert werden"},
            {"By shooting date", "Nach Aufnahmedatum"}, {"By creation date", "Nach Erstellungsdatum"}, {"Scan folder", "Ordner durchsuchen"}, {"Close cached photos", "Zwischengespeicherte Fotos schließen"},
            {"No cached photos yet", "Noch keine Fotos im Cache"}, {"No photos in this year", "Keine Fotos in diesem Jahr"}, {"Photo scan log", "Foto-Scanprotokoll"}, {"Pause", "Pausieren"}, {"Resume", "Fortsetzen"}, {"Stop", "Stopp"},
            {"Choose a folder to scan", "Ordner zum Durchsuchen auswählen"}, {"Restore hidden cached photos?", "Ausgeblendete Fotos im Cache wiederherstellen?"},
            {"If a cached photo is still present in this folder, should it be made visible in the cache browser?", "Soll ein Foto im Cache, das noch in diesem Ordner vorhanden ist, im Cachebrowser sichtbar gemacht werden?"},
            {"Scanner not found", "Scanner nicht gefunden"}, {"Could not find the scanner application:\n%1", "Scanner-Anwendung nicht gefunden:\n%1"}, {"Scanning…", "Wird durchsucht…"}, {"Scanning folder: %1", "Ordner wird durchsucht: %1"},
            {"Photo moved", "Foto verschoben"}, {"This photo was found at a new path.\n\nFrom: %1\nTo: %2\n\nRelink this photo and apply the folder change to other matching entries?", "Dieses Foto wurde an einem neuen Pfad gefunden.\n\nVon: %1\nNach: %2\n\nFoto neu verknüpfen und die Ordneränderung auf passende Einträge anwenden?"},
            {"Relink approved: %1 → %2", "Neuverknüpfung bestätigt: %1 → %2"}, {"Relink declined; aborting scan.", "Neuverknüpfung abgelehnt; Scan wird abgebrochen."}, {"Select every photo in this year", "Alle Fotos dieses Jahres auswählen"},
            {"Select every photo in this month", "Alle Fotos dieses Monats auswählen"}, {"Undated", "Ohne Datum"}, {"Could not hide some photos", "Einige Fotos konnten nicht ausgeblendet werden"}, {"… and %1 more", "… und %1 weitere"},
            {"Delete photos permanently?", "Fotos dauerhaft löschen?"}, {"This will permanently delete %n selected image file(s):\n\n%1\n\nContinue?", "Dadurch werden %n ausgewählte Bilddateien dauerhaft gelöscht:\n\n%1\n\nFortfahren?"},
            {"Could not delete file: %1", "Datei konnte nicht gelöscht werden: %1"}, {"Could not remove cache entry for %1: %2", "Cacheeintrag für %1 konnte nicht entfernt werden: %2"}, {"Some photos could not be deleted", "Einige Fotos konnten nicht gelöscht werden"},
            {"%1: %2", "%1: %2"}, {"%1: metadata saved, but hash could not be refreshed (%2)", "%1: Metadaten gespeichert, Hash konnte jedoch nicht aktualisiert werden (%2)"},
            {"%1: metadata saved, but the library entry could not be updated (%2)", "%1: Metadaten gespeichert, Bibliothekseintrag konnte jedoch nicht aktualisiert werden (%2)"},
            {"Stop requested; waiting for the current image to finish…", "Stopp angefordert; aktuelles Bild wird noch fertiggestellt…"}, {"Scanner exited with code %1.", "Scanner mit Code %1 beendet."}, {"Could not start scanner: %1", "Scanner konnte nicht gestartet werden: %1"},
            {"Photo scan log — 0.0 %", "Foto-Scanprotokoll — 0,0 %"}, {"Photo scan log — %1 %", "Foto-Scanprotokoll — %1 %"},
            {"This photo was found at a new path.\n\nFrom: %1\nTo: %2\n\nRelink this photo and use this folder change for other matching entries? Choosing No aborts this scan.", "Dieses Foto wurde an einem neuen Pfad gefunden.\n\nVon: %1\nNach: %2\n\nFoto neu verknüpfen und die Ordneränderung auf passende Einträge anwenden? Bei Nein wird der Scan abgebrochen."},
            {"Settings", "Einstellungen"}, {"Database file: %1", "Datenbankdatei: %1"}, {"Reveal database in file explorer", "Datenbank im Datei-Explorer anzeigen"},
            {"Could not open folder", "Ordner konnte nicht geöffnet werden"}, {"Could not open %1", "%1 konnte nicht geöffnet werden"},
            {"Language", "Sprache"}, {"Restart Photo Snap to apply the language change.", "Starte Photo Snap neu, um die Sprachänderung anzuwenden."}, {"Language changed", "Sprache geändert"}
        }},
        {QStringLiteral("FR"), {
            {"&File", "&Fichier"}, {"&Open…", "&Ouvrir…"}, {"&Quit", "&Quitter"}, {"&View", "&Affichage"}, {"&Navigate", "&Navigation"},
            {"Open a photo to get started · Ctrl+O", "Ouvrez une photo pour commencer · Ctrl+O"}, {"Previous photo", "Photo précédente"}, {"Next photo", "Photo suivante"}, {"Zoom in", "Zoom avant"}, {"Zoom out", "Zoom arrière"},
            {"Fit to window", "Adapter à la fenêtre"}, {"Actual size", "Taille réelle"}, {"Rotate", "Pivoter"}, {"Copy", "Copier"}, {"Full screen", "Plein écran"}, {"Exit full screen", "Quitter le plein écran"}, {"Library", "Bibliothèque"},
            {"Photo tools", "Outils photo"}, {"Open", "Ouvrir"}, {"Open photo", "Ouvrir une photo"}, {"Open library", "Ouvrir la bibliothèque"}, {"Edit metadata", "Modifier les métadonnées"},
            {"Images (%1);;All files (*)", "Images (%1);;Tous les fichiers (*)"}, {"Unable to open photo", "Impossible d’ouvrir la photo"}, {"Could not read “%1”.\n%2", "Impossible de lire « %1 ».\n%2"}, {"Image cache unavailable", "Cache d’images indisponible"},
            {"Photo copied to clipboard", "Photo copiée dans le presse-papiers"}, {"%1 selected photo(s) copied to clipboard", "%1 photo(s) sélectionnée(s) copiée(s) dans le presse-papiers"}, {"Edit selected photo metadata?", "Modifier les métadonnées des photos sélectionnées ?"},
            {"Apply the same shooting date, author, and copyright to all %n selected photos?", "Appliquer la même date de prise de vue, le même auteur et copyright aux %n photos sélectionnées ?"}, {"Update", "Mettre à jour"},
            {"When unchecked, keep each photo's existing value.", "Si cette option est décochée, conserver la valeur actuelle de chaque photo."}, {"Shot date/time", "Date/heure de prise de vue"}, {"Photographer", "Photographe"}, {"Author", "Auteur"}, {"Copyright", "Copyright"},
            {"Some library entries could not be updated", "Certaines entrées de la bibliothèque n’ont pas pu être mises à jour"}, {"Some photos could not be updated", "Certaines photos n’ont pas pu être mises à jour"},
            {"By shooting date", "Par date de prise de vue"}, {"By creation date", "Par date de création"}, {"Scan folder", "Analyser le dossier"}, {"Close cached photos", "Fermer les photos en cache"}, {"No cached photos yet", "Aucune photo en cache"},
            {"No photos in this year", "Aucune photo cette année"}, {"Photo scan log", "Journal d’analyse des photos"}, {"Pause", "Pause"}, {"Resume", "Reprendre"}, {"Stop", "Arrêter"}, {"Choose a folder to scan", "Choisir un dossier à analyser"},
            {"Restore hidden cached photos?", "Restaurer les photos masquées du cache ?"}, {"If a cached photo is still present in this folder, should it be made visible in the cache browser?", "Si une photo en cache est toujours présente dans ce dossier, doit-elle être visible dans la bibliothèque ?"},
            {"Scanner not found", "Scanner introuvable"}, {"Could not find the scanner application:\n%1", "Impossible de trouver l’application de numérisation :\n%1"}, {"Scanning…", "Analyse en cours…"}, {"Scanning folder: %1", "Analyse du dossier : %1"}, {"Photo moved", "Photo déplacée"},
            {"This photo was found at a new path.\n\nFrom: %1\nTo: %2\n\nRelink this photo and apply the folder change to other matching entries?", "Cette photo a été trouvée à un nouvel emplacement.\n\nDe : %1\nÀ : %2\n\nRelier cette photo et appliquer le changement de dossier aux autres entrées correspondantes ?"},
            {"Relink approved: %1 → %2", "Reliaison acceptée : %1 → %2"}, {"Relink declined; aborting scan.", "Reliaison refusée ; analyse interrompue."}, {"Select every photo in this year", "Sélectionner toutes les photos de cette année"}, {"Select every photo in this month", "Sélectionner toutes les photos de ce mois"}, {"Undated", "Sans date"},
            {"Could not hide some photos", "Impossible de masquer certaines photos"}, {"… and %1 more", "… et %1 de plus"}, {"Delete photos permanently?", "Supprimer définitivement les photos ?"},
            {"This will permanently delete %n selected image file(s):\n\n%1\n\nContinue?", "Cette action supprimera définitivement %n fichier(s) image sélectionné(s) :\n\n%1\n\nContinuer ?"}, {"Could not delete file: %1", "Impossible de supprimer le fichier : %1"},
            {"Could not remove cache entry for %1: %2", "Impossible de supprimer l’entrée du cache pour %1 : %2"}, {"Some photos could not be deleted", "Certaines photos n’ont pas pu être supprimées"},
            {"%1: %2", "%1 : %2"}, {"%1: metadata saved, but hash could not be refreshed (%2)", "%1 : métadonnées enregistrées, mais impossible d’actualiser le hachage (%2)"},
            {"%1: metadata saved, but the library entry could not be updated (%2)", "%1 : métadonnées enregistrées, mais impossible de mettre à jour l’entrée de la bibliothèque (%2)"},
            {"Stop requested; waiting for the current image to finish…", "Arrêt demandé ; attente de la fin de l’image en cours…"}, {"Scanner exited with code %1.", "Le scanner s’est terminé avec le code %1."}, {"Could not start scanner: %1", "Impossible de démarrer le scanner : %1"},
            {"Photo scan log — 0.0 %", "Journal d’analyse des photos — 0,0 %"}, {"Photo scan log — %1 %", "Journal d’analyse des photos — %1 %"},
            {"This photo was found at a new path.\n\nFrom: %1\nTo: %2\n\nRelink this photo and use this folder change for other matching entries? Choosing No aborts this scan.", "Cette photo a été trouvée à un nouvel emplacement.\n\nDe : %1\nÀ : %2\n\nRelier cette photo et appliquer le changement de dossier aux autres entrées correspondantes ? Choisir Non interrompra l’analyse."},
            {"Settings", "Paramètres"}, {"Database file: %1", "Fichier de base de données : %1"}, {"Reveal database in file explorer", "Afficher la base de données dans le dossier"},
            {"Could not open folder", "Impossible d’ouvrir le dossier"}, {"Could not open %1", "Impossible d’ouvrir %1"},
            {"Language", "Langue"}, {"Restart Photo Snap to apply the language change.", "Redémarrez Photo Snap pour appliquer le changement de langue."}, {"Language changed", "Langue modifiée"}
        }},
        {QStringLiteral("ES"), {
            {"&File", "&Archivo"}, {"&Open…", "&Abrir…"}, {"&Quit", "&Salir"}, {"&View", "&Ver"}, {"&Navigate", "&Navegar"},
            {"Open a photo to get started · Ctrl+O", "Abre una foto para empezar · Ctrl+O"}, {"Previous photo", "Foto anterior"}, {"Next photo", "Foto siguiente"}, {"Zoom in", "Acercar"}, {"Zoom out", "Alejar"}, {"Fit to window", "Ajustar a la ventana"}, {"Actual size", "Tamaño real"},
            {"Rotate", "Girar"}, {"Copy", "Copiar"}, {"Full screen", "Pantalla completa"}, {"Exit full screen", "Salir de pantalla completa"}, {"Library", "Biblioteca"}, {"Photo tools", "Herramientas de fotos"}, {"Open", "Abrir"}, {"Open photo", "Abrir foto"},
            {"Open library", "Abrir biblioteca"}, {"Edit metadata", "Editar metadatos"}, {"Images (%1);;All files (*)", "Imágenes (%1);;Todos los archivos (*)"}, {"Unable to open photo", "No se puede abrir la foto"}, {"Could not read “%1”.\n%2", "No se pudo leer “%1”.\n%2"},
            {"Image cache unavailable", "Caché de imágenes no disponible"}, {"Photo copied to clipboard", "Foto copiada al portapapeles"}, {"%1 selected photo(s) copied to clipboard", "%1 foto(s) seleccionada(s) copiada(s) al portapapeles"}, {"Edit selected photo metadata?", "¿Editar los metadatos de las fotos seleccionadas?"},
            {"Apply the same shooting date, author, and copyright to all %n selected photos?", "¿Aplicar la misma fecha de captura, autor y copyright a las %n fotos seleccionadas?"}, {"Update", "Actualizar"}, {"When unchecked, keep each photo's existing value.", "Si no está marcado, se conserva el valor actual de cada foto."}, {"Shot date/time", "Fecha y hora de captura"}, {"Photographer", "Fotógrafo"}, {"Author", "Autor"}, {"Copyright", "Copyright"},
            {"Some library entries could not be updated", "No se pudieron actualizar algunas entradas de la biblioteca"}, {"Some photos could not be updated", "No se pudieron actualizar algunas fotos"}, {"By shooting date", "Por fecha de captura"}, {"By creation date", "Por fecha de creación"}, {"Scan folder", "Analizar carpeta"}, {"Close cached photos", "Cerrar fotos en caché"}, {"No cached photos yet", "Aún no hay fotos en caché"},
            {"No photos in this year", "No hay fotos este año"}, {"Photo scan log", "Registro de análisis de fotos"}, {"Pause", "Pausar"}, {"Resume", "Reanudar"}, {"Stop", "Detener"}, {"Choose a folder to scan", "Elige una carpeta para analizar"}, {"Restore hidden cached photos?", "¿Restaurar las fotos ocultas de la caché?"},
            {"If a cached photo is still present in this folder, should it be made visible in the cache browser?", "Si una foto en caché sigue en esta carpeta, ¿debe mostrarse en el explorador de caché?"}, {"Scanner not found", "No se encontró el escáner"}, {"Could not find the scanner application:\n%1", "No se encontró la aplicación de análisis:\n%1"}, {"Scanning…", "Analizando…"}, {"Scanning folder: %1", "Analizando carpeta: %1"}, {"Photo moved", "Foto movida"},
            {"This photo was found at a new path.\n\nFrom: %1\nTo: %2\n\nRelink this photo and apply the folder change to other matching entries?", "Se encontró esta foto en una nueva ruta.\n\nDe: %1\nA: %2\n\n¿Volver a vincularla y aplicar el cambio de carpeta a las demás entradas coincidentes?"},
            {"Relink approved: %1 → %2", "Vínculo aprobado: %1 → %2"}, {"Relink declined; aborting scan.", "Vínculo rechazado; se cancela el análisis."}, {"Select every photo in this year", "Seleccionar todas las fotos de este año"}, {"Select every photo in this month", "Seleccionar todas las fotos de este mes"}, {"Undated", "Sin fecha"},
            {"Could not hide some photos", "No se pudieron ocultar algunas fotos"}, {"… and %1 more", "… y %1 más"}, {"Delete photos permanently?", "¿Eliminar las fotos permanentemente?"}, {"This will permanently delete %n selected image file(s):\n\n%1\n\nContinue?", "Esto eliminará permanentemente %n archivo(s) de imagen seleccionado(s):\n\n%1\n\n¿Continuar?"},
            {"Could not delete file: %1", "No se pudo eliminar el archivo: %1"}, {"Could not remove cache entry for %1: %2", "No se pudo quitar de la caché la entrada de %1: %2"}, {"Some photos could not be deleted", "No se pudieron eliminar algunas fotos"},
            {"%1: %2", "%1: %2"}, {"%1: metadata saved, but hash could not be refreshed (%2)", "%1: se guardaron los metadatos, pero no se pudo actualizar el hash (%2)"},
            {"%1: metadata saved, but the library entry could not be updated (%2)", "%1: se guardaron los metadatos, pero no se pudo actualizar la entrada de la biblioteca (%2)"},
            {"Stop requested; waiting for the current image to finish…", "Se solicitó detener; esperando a que termine la imagen actual…"}, {"Scanner exited with code %1.", "El escáner terminó con el código %1."}, {"Could not start scanner: %1", "No se pudo iniciar el escáner: %1"},
            {"Photo scan log — 0.0 %", "Registro de análisis de fotos — 0,0 %"}, {"Photo scan log — %1 %", "Registro de análisis de fotos — %1 %"},
            {"This photo was found at a new path.\n\nFrom: %1\nTo: %2\n\nRelink this photo and use this folder change for other matching entries? Choosing No aborts this scan.", "Se encontró esta foto en una nueva ruta.\n\nDe: %1\nA: %2\n\n¿Volver a vincularla y aplicar el cambio de carpeta a las demás entradas coincidentes? Si eliges No, se cancelará el análisis."},
            {"Settings", "Ajustes"}, {"Database file: %1", "Archivo de base de datos: %1"}, {"Reveal database in file explorer", "Mostrar la base de datos en la carpeta"},
            {"Could not open folder", "No se pudo abrir la carpeta"}, {"Could not open %1", "No se pudo abrir %1"},
            {"Language", "Idioma"}, {"Restart Photo Snap to apply the language change.", "Reinicia Photo Snap para aplicar el cambio de idioma."}, {"Language changed", "Idioma cambiado"}
        }}
    };
    return value;
}
} // namespace

QStringList supportedLanguages()
{
    return {QStringLiteral("EN-GB"), QStringLiteral("EN-US"), QStringLiteral("IT"),
            QStringLiteral("DE"), QStringLiteral("FR"), QStringLiteral("ES")};
}

QString languageName(const QString &language)
{
    static const QHash<QString, QString> names = {
        {"EN-GB", "English (UK)"}, {"EN-US", "English (US)"}, {"IT", "Italiano"},
        {"DE", "Deutsch"}, {"FR", "Français"}, {"ES", "Español"}
    };
    return names.value(normalizedLanguage(language), QStringLiteral("English (UK)"));
}

QString normalizedLanguage(const QString &language)
{
    const QString code = language.trimmed().toUpper().replace(QLatin1Char('_'), QLatin1Char('-'));
    if (code == QLatin1String("EN-US") || code == QLatin1String("EN-GB") || code == QLatin1String("IT") ||
        code == QLatin1String("DE") || code == QLatin1String("FR") || code == QLatin1String("ES"))
        return code;
    return QStringLiteral("EN-GB");
}

AppTranslator::AppTranslator(QString language, QObject *parent)
    : QTranslator(parent), m_language(normalizedLanguage(language))
{
}

QString AppTranslator::translate(const char *, const char *sourceText, const char *, int n) const
{
    if (!sourceText || m_language.startsWith(QLatin1String("EN-")))
        return {};
    const QString source = QString::fromUtf8(sourceText);
    const QString translated = catalogs().value(m_language).value(source);
    if (translated.isNull())
        return {};
    return n >= 0 ? QString(translated).replace(QStringLiteral("%n"), QString::number(n)) : translated;
}
