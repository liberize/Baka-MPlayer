#include "util.h"

#include <QTime>
#include <QStringListIterator>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

namespace Util {

const QHash<QString, QString> iso639CodeMap = {
    // ISO639_1
    {"aa", "Afar"},
    {"ab", "Abkhazian"},
    {"af", "Afrikaans"},
    {"ak", "Akan"},
    {"sq", "Albanian"},
    {"am", "Amharic"},
    {"ar", "Arabic"},
    {"an", "Aragonese"},
    {"hy", "Armenian"},
    {"as", "Assamese"},
    {"av", "Avaric"},
    {"ae", "Avestan"},
    {"ay", "Aymara"},
    {"az", "Azerbaijani"},
    {"ba", "Bashkir"},
    {"bm", "Bambara"},
    {"eu", "Basque"},
    {"be", "Belarusian"},
    {"bn", "Bengali"},
    {"bh", "Bihari languages"},
    {"bi", "Bislama"},
    {"bs", "Bosnian"},
    {"br", "Breton"},
    {"bg", "Bulgarian"},
    {"my", "Burmese"},
    {"ca", "Catalan; Valencian"},
    {"ch", "Chamorro"},
    {"ce", "Chechen"},
    {"zh", "Chinese"},
    {"cu", "Church Slavic; Old Slavonic; Church Slavonic; Old Bulgarian; Old Church Slavonic"},
    {"cv", "Chuvash"},
    {"kw", "Cornish"},
    {"co", "Corsican"},
    {"cr", "Cree"},
    {"cs", "Czech"},
    {"da", "Danish"},
    {"dv", "Divehi; Dhivehi; Maldivian"},
    {"nl", "Dutch; Flemish"},
    {"dz", "Dzongkha"},
    {"en", "English"},
    {"eo", "Esperanto"},
    {"et", "Estonian"},
    {"ee", "Ewe"},
    {"fo", "Faroese"},
    {"fj", "Fijian"},
    {"fi", "Finnish"},
    {"fr", "French"},
    {"fy", "Western Frisian"},
    {"ff", "Fulah"},
    {"ka", "Georgian"},
    {"de", "German"},
    {"gd", "Gaelic; Scottish Gaelic"},
    {"ga", "Irish"},
    {"gl", "Galician"},
    {"gv", "Manx"},
    {"el", "Greek, Modern (1453-)"},
    {"gn", "Guarani"},
    {"gu", "Gujarati"},
    {"ht", "Haitian; Haitian Creole"},
    {"ha", "Hausa"},
    {"he", "Hebrew"},
    {"hz", "Herero"},
    {"hi", "Hindi"},
    {"ho", "Hiri Motu"},
    {"hr", "Croatian"},
    {"hu", "Hungarian"},
    {"ig", "Igbo"},
    {"is", "Icelandic"},
    {"io", "Ido"},
    {"ii", "Sichuan Yi; Nuosu"},
    {"iu", "Inuktitut"},
    {"ie", "Interlingue; Occidental"},
    {"ia", "Interlingua (International Auxiliary Language Association)"},
    {"id", "Indonesian"},
    {"ik", "Inupiaq"},
    {"it", "Italian"},
    {"jv", "Javanese"},
    {"ja", "Japanese"},
    {"kl", "Kalaallisut; Greenlandic"},
    {"kn", "Kannada"},
    {"ks", "Kashmiri"},
    {"kr", "Kanuri"},
    {"kk", "Kazakh"},
    {"km", "Central Khmer"},
    {"ki", "Kikuyu; Gikuyu"},
    {"rw", "Kinyarwanda"},
    {"ky", "Kirghiz; Kyrgyz"},
    {"kv", "Komi"},
    {"kg", "Kongo"},
    {"ko", "Korean"},
    {"kj", "Kuanyama; Kwanyama"},
    {"ku", "Kurdish"},
    {"lo", "Lao"},
    {"la", "Latin"},
    {"lv", "Latvian"},
    {"li", "Limburgan; Limburger; Limburgish"},
    {"ln", "Lingala"},
    {"lt", "Lithuanian"},
    {"lb", "Luxembourgish; Letzeburgesch"},
    {"lu", "Luba-Katanga"},
    {"lg", "Ganda"},
    {"mk", "Macedonian"},
    {"mh", "Marshallese"},
    {"ml", "Malayalam"},
    {"mi", "Maori"},
    {"mr", "Marathi"},
    {"ms", "Malay"},
    {"mg", "Malagasy"},
    {"mt", "Maltese"},
    {"mn", "Mongolian"},
    {"na", "Nauru"},
    {"nv", "Navajo; Navaho"},
    {"nr", "Ndebele, South; South Ndebele"},
    {"nd", "Ndebele, North; North Ndebele"},
    {"ng", "Ndonga"},
    {"ne", "Nepali"},
    {"nn", "Norwegian Nynorsk; Nynorsk, Norwegian"},
    {"nb", "Bokmål, Norwegian; Norwegian Bokmål"},
    {"no", "Norwegian"},
    {"ny", "Chichewa; Chewa; Nyanja"},
    {"oc", "Occitan (post 1500); Provençal"},
    {"oj", "Ojibwa"},
    {"or", "Oriya"},
    {"om", "Oromo"},
    {"os", "Ossetian; Ossetic"},
    {"pa", "Panjabi; Punjabi"},
    {"fa", "Persian"},
    {"pi", "Pali"},
    {"pl", "Polish"},
    {"pt", "Portuguese"},
    {"ps", "Pushto; Pashto"},
    {"qu", "Quechua"},
    {"rm", "Romansh"},
    {"ro", "Romanian; Moldavian; Moldovan"},
    {"rn", "Rundi"},
    {"ru", "Russian"},
    {"sg", "Sango"},
    {"sa", "Sanskrit"},
    {"si", "Sinhala; Sinhalese"},
    {"sk", "Slovak"},
    {"sl", "Slovenian"},
    {"se", "Northern Sami"},
    {"sm", "Samoan"},
    {"sn", "Shona"},
    {"sd", "Sindhi"},
    {"so", "Somali"},
    {"st", "Sotho, Southern"},
    {"es", "Spanish; Castilian"},
    {"sc", "Sardinian"},
    {"sr", "Serbian"},
    {"ss", "Swati"},
    {"su", "Sundanese"},
    {"sw", "Swahili"},
    {"sv", "Swedish"},
    {"ty", "Tahitian"},
    {"ta", "Tamil"},
    {"tt", "Tatar"},
    {"te", "Telugu"},
    {"tg", "Tajik"},
    {"tl", "Tagalog"},
    {"th", "Thai"},
    {"bo", "Tibetan"},
    {"ti", "Tigrinya"},
    {"to", "Tonga (Tonga Islands)"},
    {"tn", "Tswana"},
    {"ts", "Tsonga"},
    {"tk", "Turkmen"},
    {"tr", "Turkish"},
    {"tw", "Twi"},
    {"ug", "Uighur; Uyghur"},
    {"uk", "Ukrainian"},
    {"ur", "Urdu"},
    {"uz", "Uzbek"},
    {"ve", "Venda"},
    {"vi", "Vietnamese"},
    {"vo", "Volapük"},
    {"cy", "Welsh"},
    {"wa", "Walloon"},
    {"wo", "Wolof"},
    {"xh", "Xhosa"},
    {"yi", "Yiddish"},
    {"yo", "Yoruba"},
    {"za", "Zhuang; Chuang"},
    {"zu", "Zulu"},

    // ISO639_2 B/T
    {"aar", "Afar"},
    {"abk", "Abkhazian"},
    {"ace", "Achinese"},
    {"ach", "Acoli"},
    {"ada", "Adangme"},
    {"ady", "Adyghe; Adygei"},
    {"afa", "Afro-Asiatic languages"},
    {"afh", "Afrihili"},
    {"afr", "Afrikaans"},
    {"ain", "Ainu"},
    {"aka", "Akan"},
    {"akk", "Akkadian"},
    {"alb", "Albanian"},
    {"sqi", "Albanian"},
    {"ale", "Aleut"},
    {"alg", "Algonquian languages"},
    {"alt", "Southern Altai"},
    {"amh", "Amharic"},
    {"ang", "English, Old (ca.450-1100)"},
    {"anp", "Angika"},
    {"apa", "Apache languages"},
    {"ara", "Arabic"},
    {"arc", "Official Aramaic (700-300 BCE); Imperial Aramaic (700-300 BCE)"},
    {"arg", "Aragonese"},
    {"arm", "Armenian"},
    {"hye", "Armenian"},
    {"arn", "Mapudungun; Mapuche"},
    {"arp", "Arapaho"},
    {"art", "Artificial languages"},
    {"arw", "Arawak"},
    {"asm", "Assamese"},
    {"ast", "Asturian; Bable; Leonese; Asturleonese"},
    {"ath", "Athapascan languages"},
    {"aus", "Australian languages"},
    {"ava", "Avaric"},
    {"ave", "Avestan"},
    {"awa", "Awadhi"},
    {"aym", "Aymara"},
    {"aze", "Azerbaijani"},
    {"bad", "Banda languages"},
    {"bai", "Bamileke languages"},
    {"bak", "Bashkir"},
    {"bal", "Baluchi"},
    {"bam", "Bambara"},
    {"ban", "Balinese"},
    {"baq", "Basque"},
    {"eus", "Basque"},
    {"bas", "Basa"},
    {"bat", "Baltic languages"},
    {"bej", "Beja; Bedawiyet"},
    {"bel", "Belarusian"},
    {"bem", "Bemba"},
    {"ben", "Bengali"},
    {"ber", "Berber languages"},
    {"bho", "Bhojpuri"},
    {"bih", "Bihari languages"},
    {"bik", "Bikol"},
    {"bin", "Bini; Edo"},
    {"bis", "Bislama"},
    {"bla", "Siksika"},
    {"bnt", "Bantu (Other)"},
    {"bos", "Bosnian"},
    {"bra", "Braj"},
    {"bre", "Breton"},
    {"btk", "Batak languages"},
    {"bua", "Buriat"},
    {"bug", "Buginese"},
    {"bul", "Bulgarian"},
    {"bur", "Burmese"},
    {"mya", "Burmese"},
    {"byn", "Blin; Bilin"},
    {"cad", "Caddo"},
    {"cai", "Central American Indian languages"},
    {"car", "Galibi Carib"},
    {"cat", "Catalan; Valencian"},
    {"cau", "Caucasian languages"},
    {"ceb", "Cebuano"},
    {"cel", "Celtic languages"},
    {"cha", "Chamorro"},
    {"chb", "Chibcha"},
    {"che", "Chechen"},
    {"chg", "Chagatai"},
    {"chi", "Chinese"},
    {"zho", "Chinese"},
    {"chk", "Chuukese"},
    {"chm", "Mari"},
    {"chn", "Chinook jargon"},
    {"cho", "Choctaw"},
    {"chp", "Chipewyan; Dene Suline"},
    {"chr", "Cherokee"},
    {"chu", "Church Slavic; Old Slavonic; Church Slavonic; Old Bulgarian; Old Church Slavonic"},
    {"chv", "Chuvash"},
    {"chy", "Cheyenne"},
    {"cmc", "Chamic languages"},
    {"cop", "Coptic"},
    {"cor", "Cornish"},
    {"cos", "Corsican"},
    {"cpe", "Creoles and pidgins, English based"},
    {"cpf", "Creoles and pidgins, French-based "},
    {"cpp", "Creoles and pidgins, Portuguese-based "},
    {"cre", "Cree"},
    {"crh", "Crimean Tatar; Crimean Turkish"},
    {"crp", "Creoles and pidgins "},
    {"csb", "Kashubian"},
    {"cus", "Cushitic languages"},
    {"cze", "Czech"},
    {"ces", "Czech"},
    {"dak", "Dakota"},
    {"dan", "Danish"},
    {"dar", "Dargwa"},
    {"day", "Land Dayak languages"},
    {"del", "Delaware"},
    {"den", "Slave (Athapascan)"},
    {"dgr", "Dogrib"},
    {"din", "Dinka"},
    {"div", "Divehi; Dhivehi; Maldivian"},
    {"doi", "Dogri"},
    {"dra", "Dravidian languages"},
    {"dsb", "Lower Sorbian"},
    {"dua", "Duala"},
    {"dum", "Dutch, Middle (ca.1050-1350)"},
    {"dut", "Dutch; Flemish"},
    {"nld", "Dutch; Flemish"},
    {"dyu", "Dyula"},
    {"dzo", "Dzongkha"},
    {"efi", "Efik"},
    {"egy", "Egyptian (Ancient)"},
    {"eka", "Ekajuk"},
    {"elx", "Elamite"},
    {"eng", "English"},
    {"enm", "English, Middle (1100-1500)"},
    {"epo", "Esperanto"},
    {"est", "Estonian"},
    {"ewe", "Ewe"},
    {"ewo", "Ewondo"},
    {"fan", "Fang"},
    {"fao", "Faroese"},
    {"fat", "Fanti"},
    {"fij", "Fijian"},
    {"fil", "Filipino; Pilipino"},
    {"fin", "Finnish"},
    {"fiu", "Finno-Ugrian languages"},
    {"fon", "Fon"},
    {"fre", "French"},
    {"fra", "French"},
    {"frm", "French, Middle (ca.1400-1600)"},
    {"fro", "French, Old (842-ca.1400)"},
    {"frr", "Northern Frisian"},
    {"frs", "Eastern Frisian"},
    {"fry", "Western Frisian"},
    {"ful", "Fulah"},
    {"fur", "Friulian"},
    {"gaa", "Ga"},
    {"gay", "Gayo"},
    {"gba", "Gbaya"},
    {"gem", "Germanic languages"},
    {"geo", "Georgian"},
    {"kat", "Georgian"},
    {"ger", "German"},
    {"deu", "German"},
    {"gez", "Geez"},
    {"gil", "Gilbertese"},
    {"gla", "Gaelic; Scottish Gaelic"},
    {"gle", "Irish"},
    {"glg", "Galician"},
    {"glv", "Manx"},
    {"gmh", "German, Middle High (ca.1050-1500)"},
    {"goh", "German, Old High (ca.750-1050)"},
    {"gon", "Gondi"},
    {"gor", "Gorontalo"},
    {"got", "Gothic"},
    {"grb", "Grebo"},
    {"grc", "Greek, Ancient (to 1453)"},
    {"gre", "Greek, Modern (1453-)"},
    {"ell", "Greek, Modern (1453-)"},
    {"grn", "Guarani"},
    {"gsw", "Swiss German; Alemannic; Alsatian"},
    {"guj", "Gujarati"},
    {"gwi", "Gwich'in"},
    {"hai", "Haida"},
    {"hat", "Haitian; Haitian Creole"},
    {"hau", "Hausa"},
    {"haw", "Hawaiian"},
    {"heb", "Hebrew"},
    {"her", "Herero"},
    {"hil", "Hiligaynon"},
    {"him", "Himachali languages; Western Pahari languages"},
    {"hin", "Hindi"},
    {"hit", "Hittite"},
    {"hmn", "Hmong; Mong"},
    {"hmo", "Hiri Motu"},
    {"hrv", "Croatian"},
    {"hsb", "Upper Sorbian"},
    {"hun", "Hungarian"},
    {"hup", "Hupa"},
    {"iba", "Iban"},
    {"ibo", "Igbo"},
    {"ice", "Icelandic"},
    {"isl", "Icelandic"},
    {"ido", "Ido"},
    {"iii", "Sichuan Yi; Nuosu"},
    {"ijo", "Ijo languages"},
    {"iku", "Inuktitut"},
    {"ile", "Interlingue; Occidental"},
    {"ilo", "Iloko"},
    {"ina", "Interlingua (International Auxiliary Language Association)"},
    {"inc", "Indic languages"},
    {"ind", "Indonesian"},
    {"ine", "Indo-European languages"},
    {"inh", "Ingush"},
    {"ipk", "Inupiaq"},
    {"ira", "Iranian languages"},
    {"iro", "Iroquoian languages"},
    {"ita", "Italian"},
    {"jav", "Javanese"},
    {"jbo", "Lojban"},
    {"jpn", "Japanese"},
    {"jpr", "Judeo-Persian"},
    {"jrb", "Judeo-Arabic"},
    {"kaa", "Kara-Kalpak"},
    {"kab", "Kabyle"},
    {"kac", "Kachin; Jingpho"},
    {"kal", "Kalaallisut; Greenlandic"},
    {"kam", "Kamba"},
    {"kan", "Kannada"},
    {"kar", "Karen languages"},
    {"kas", "Kashmiri"},
    {"kau", "Kanuri"},
    {"kaw", "Kawi"},
    {"kaz", "Kazakh"},
    {"kbd", "Kabardian"},
    {"kha", "Khasi"},
    {"khi", "Khoisan languages"},
    {"khm", "Central Khmer"},
    {"kho", "Khotanese; Sakan"},
    {"kik", "Kikuyu; Gikuyu"},
    {"kin", "Kinyarwanda"},
    {"kir", "Kirghiz; Kyrgyz"},
    {"kmb", "Kimbundu"},
    {"kok", "Konkani"},
    {"kom", "Komi"},
    {"kon", "Kongo"},
    {"kor", "Korean"},
    {"kos", "Kosraean"},
    {"kpe", "Kpelle"},
    {"krc", "Karachay-Balkar"},
    {"krl", "Karelian"},
    {"kro", "Kru languages"},
    {"kru", "Kurukh"},
    {"kua", "Kuanyama; Kwanyama"},
    {"kum", "Kumyk"},
    {"kur", "Kurdish"},
    {"kut", "Kutenai"},
    {"lad", "Ladino"},
    {"lah", "Lahnda"},
    {"lam", "Lamba"},
    {"lao", "Lao"},
    {"lat", "Latin"},
    {"lav", "Latvian"},
    {"lez", "Lezghian"},
    {"lim", "Limburgan; Limburger; Limburgish"},
    {"lin", "Lingala"},
    {"lit", "Lithuanian"},
    {"lol", "Mongo"},
    {"loz", "Lozi"},
    {"ltz", "Luxembourgish; Letzeburgesch"},
    {"lua", "Luba-Lulua"},
    {"lub", "Luba-Katanga"},
    {"lug", "Ganda"},
    {"lui", "Luiseno"},
    {"lun", "Lunda"},
    {"luo", "Luo (Kenya and Tanzania)"},
    {"lus", "Lushai"},
    {"mac", "Macedonian"},
    {"mkd", "Macedonian"},
    {"mad", "Madurese"},
    {"mag", "Magahi"},
    {"mah", "Marshallese"},
    {"mai", "Maithili"},
    {"mak", "Makasar"},
    {"mal", "Malayalam"},
    {"man", "Mandingo"},
    {"mao", "Maori"},
    {"mri", "Maori"},
    {"map", "Austronesian languages"},
    {"mar", "Marathi"},
    {"mas", "Masai"},
    {"may", "Malay"},
    {"msa", "Malay"},
    {"mdf", "Moksha"},
    {"mdr", "Mandar"},
    {"men", "Mende"},
    {"mga", "Irish, Middle (900-1200)"},
    {"mic", "Mi'kmaq; Micmac"},
    {"min", "Minangkabau"},
    {"mis", "Uncoded languages"},
    {"mkh", "Mon-Khmer languages"},
    {"mlg", "Malagasy"},
    {"mlt", "Maltese"},
    {"mnc", "Manchu"},
    {"mni", "Manipuri"},
    {"mno", "Manobo languages"},
    {"moh", "Mohawk"},
    {"mon", "Mongolian"},
    {"mos", "Mossi"},
    {"mul", "Multiple languages"},
    {"mun", "Munda languages"},
    {"mus", "Creek"},
    {"mwl", "Mirandese"},
    {"mwr", "Marwari"},
    {"myn", "Mayan languages"},
    {"myv", "Erzya"},
    {"nah", "Nahuatl languages"},
    {"nai", "North American Indian languages"},
    {"nap", "Neapolitan"},
    {"nau", "Nauru"},
    {"nav", "Navajo; Navaho"},
    {"nbl", "Ndebele, South; South Ndebele"},
    {"nde", "Ndebele, North; North Ndebele"},
    {"ndo", "Ndonga"},
    {"nds", "Low German; Low Saxon; German, Low; Saxon, Low"},
    {"nep", "Nepali"},
    {"new", "Nepal Bhasa; Newari"},
    {"nia", "Nias"},
    {"nic", "Niger-Kordofanian languages"},
    {"niu", "Niuean"},
    {"nno", "Norwegian Nynorsk; Nynorsk, Norwegian"},
    {"nob", "Bokmål, Norwegian; Norwegian Bokmål"},
    {"nog", "Nogai"},
    {"non", "Norse, Old"},
    {"nor", "Norwegian"},
    {"nqo", "N'Ko"},
    {"nso", "Pedi; Sepedi; Northern Sotho"},
    {"nub", "Nubian languages"},
    {"nwc", "Classical Newari; Old Newari; Classical Nepal Bhasa"},
    {"nya", "Chichewa; Chewa; Nyanja"},
    {"nym", "Nyamwezi"},
    {"nyn", "Nyankole"},
    {"nyo", "Nyoro"},
    {"nzi", "Nzima"},
    {"oci", "Occitan (post 1500); Provençal"},
    {"oji", "Ojibwa"},
    {"ori", "Oriya"},
    {"orm", "Oromo"},
    {"osa", "Osage"},
    {"oss", "Ossetian; Ossetic"},
    {"ota", "Turkish, Ottoman (1500-1928)"},
    {"oto", "Otomian languages"},
    {"paa", "Papuan languages"},
    {"pag", "Pangasinan"},
    {"pal", "Pahlavi"},
    {"pam", "Pampanga; Kapampangan"},
    {"pan", "Panjabi; Punjabi"},
    {"pap", "Papiamento"},
    {"pau", "Palauan"},
    {"peo", "Persian, Old (ca.600-400 B.C.)"},
    {"per", "Persian"},
    {"fas", "Persian"},
    {"phi", "Philippine languages"},
    {"phn", "Phoenician"},
    {"pli", "Pali"},
    {"pol", "Polish"},
    {"pon", "Pohnpeian"},
    {"por", "Portuguese"},
    {"pra", "Prakrit languages"},
    {"pro", "Provençal, Old (to 1500)"},
    {"pus", "Pushto; Pashto"},
    {"que", "Quechua"},
    {"raj", "Rajasthani"},
    {"rap", "Rapanui"},
    {"rar", "Rarotongan; Cook Islands Maori"},
    {"roa", "Romance languages"},
    {"roh", "Romansh"},
    {"rom", "Romany"},
    {"rum", "Romanian; Moldavian; Moldovan"},
    {"ron", "Romanian; Moldavian; Moldovan"},
    {"run", "Rundi"},
    {"rup", "Aromanian; Arumanian; Macedo-Romanian"},
    {"rus", "Russian"},
    {"sad", "Sandawe"},
    {"sag", "Sango"},
    {"sah", "Yakut"},
    {"sai", "South American Indian (Other)"},
    {"sal", "Salishan languages"},
    {"sam", "Samaritan Aramaic"},
    {"san", "Sanskrit"},
    {"sas", "Sasak"},
    {"sat", "Santali"},
    {"scn", "Sicilian"},
    {"sco", "Scots"},
    {"sel", "Selkup"},
    {"sem", "Semitic languages"},
    {"sga", "Irish, Old (to 900)"},
    {"sgn", "Sign Languages"},
    {"shn", "Shan"},
    {"sid", "Sidamo"},
    {"sin", "Sinhala; Sinhalese"},
    {"sio", "Siouan languages"},
    {"sit", "Sino-Tibetan languages"},
    {"sla", "Slavic languages"},
    {"slo", "Slovak"},
    {"slk", "Slovak"},
    {"slv", "Slovenian"},
    {"sma", "Southern Sami"},
    {"sme", "Northern Sami"},
    {"smi", "Sami languages"},
    {"smj", "Lule Sami"},
    {"smn", "Inari Sami"},
    {"smo", "Samoan"},
    {"sms", "Skolt Sami"},
    {"sna", "Shona"},
    {"snd", "Sindhi"},
    {"snk", "Soninke"},
    {"sog", "Sogdian"},
    {"som", "Somali"},
    {"son", "Songhai languages"},
    {"sot", "Sotho, Southern"},
    {"spa", "Spanish; Castilian"},
    {"srd", "Sardinian"},
    {"srn", "Sranan Tongo"},
    {"srp", "Serbian"},
    {"srr", "Serer"},
    {"ssa", "Nilo-Saharan languages"},
    {"ssw", "Swati"},
    {"suk", "Sukuma"},
    {"sun", "Sundanese"},
    {"sus", "Susu"},
    {"sux", "Sumerian"},
    {"swa", "Swahili"},
    {"swe", "Swedish"},
    {"syc", "Classical Syriac"},
    {"syr", "Syriac"},
    {"tah", "Tahitian"},
    {"tai", "Tai languages"},
    {"tam", "Tamil"},
    {"tat", "Tatar"},
    {"tel", "Telugu"},
    {"tem", "Timne"},
    {"ter", "Tereno"},
    {"tet", "Tetum"},
    {"tgk", "Tajik"},
    {"tgl", "Tagalog"},
    {"tha", "Thai"},
    {"tib", "Tibetan"},
    {"bod", "Tibetan"},
    {"tig", "Tigre"},
    {"tir", "Tigrinya"},
    {"tiv", "Tiv"},
    {"tkl", "Tokelau"},
    {"tlh", "Klingon; tlhIngan-Hol"},
    {"tli", "Tlingit"},
    {"tmh", "Tamashek"},
    {"tog", "Tonga (Nyasa)"},
    {"ton", "Tonga (Tonga Islands)"},
    {"tpi", "Tok Pisin"},
    {"tsi", "Tsimshian"},
    {"tsn", "Tswana"},
    {"tso", "Tsonga"},
    {"tuk", "Turkmen"},
    {"tum", "Tumbuka"},
    {"tup", "Tupi languages"},
    {"tur", "Turkish"},
    {"tut", "Altaic languages"},
    {"tvl", "Tuvalu"},
    {"twi", "Twi"},
    {"tyv", "Tuvinian"},
    {"udm", "Udmurt"},
    {"uga", "Ugaritic"},
    {"uig", "Uighur; Uyghur"},
    {"ukr", "Ukrainian"},
    {"umb", "Umbundu"},
    {"und", "Undetermined"},
    {"urd", "Urdu"},
    {"uzb", "Uzbek"},
    {"vai", "Vai"},
    {"ven", "Venda"},
    {"vie", "Vietnamese"},
    {"vol", "Volapük"},
    {"vot", "Votic"},
    {"wak", "Wakashan languages"},
    {"wal", "Walamo"},
    {"war", "Waray"},
    {"was", "Washo"},
    {"wel", "Welsh"},
    {"cym", "Welsh"},
    {"wen", "Sorbian languages"},
    {"wln", "Walloon"},
    {"wol", "Wolof"},
    {"xal", "Kalmyk; Oirat"},
    {"xho", "Xhosa"},
    {"yao", "Yao"},
    {"yap", "Yapese"},
    {"yid", "Yiddish"},
    {"yor", "Yoruba"},
    {"ypk", "Yupik languages"},
    {"zap", "Zapotec"},
    {"zbl", "Blissymbols; Blissymbolics; Bliss"},
    {"zen", "Zenaga"},
    {"zgh", "Standard Moroccan Tamazight"},
    {"zha", "Zhuang; Chuang"},
    {"znd", "Zande languages"},
    {"zul", "Zulu"},
    {"zun", "Zuni"},
    {"zxx", "No linguistic content; Not applicable"},
    {"zza", "Zaza; Dimili; Dimli; Kirdki; Kirmanjki; Zazaki"}
};

const QList<QPair<QString, QString> > charEncodingMap = {
    {"auto", QObject::tr("Auto Detect")},
    {"UTF-8", "Universal (UTF-8)"},
    {"UTF-16", "Universal (UTF-16)"},
    {"UTF-16BE", "Universal (UTF-16BE)"},
    {"UTF-16LE", "Universal (UTF-16LE)"},
    {"ISO-8859-6", "Arabic (ISO-8859-6)"},
    {"WINDOWS-1256", "Arabic (WINDOWS-1256)"},
    {"LATIN7", "Baltic (LATIN7)"},
    {"WINDOWS-1257", "Baltic (WINDOWS-1257)"},
    {"LATIN8", "Celtic (LATIN8)"},
    {"WINDOWS-1250", "Central European (WINDOWS-1250)"},
    {"ISO-8859-5", "Cyrillic (ISO-8859-5)"},
    {"WINDOWS-1251", "Cyrillic (WINDOWS-1251)"},
    {"ISO-8859-2", "Eastern European (ISO-8859-2)"},
    {"WINDOWS-1252", "Western Languages (WINDOWS-1252)"},
    {"ISO-8859-7", "Greek (ISO-8859-7)"},
    {"WINDOWS-1253", "Greek (WINDOWS-1253)"},
    {"ISO-8859-8", "Hebrew (ISO-8859-8)"},
    {"WINDOWS-1255", "Hebrew (WINDOWS-1255)"},
    {"SHIFT-JIS", "Japanese (SHIFT-JIS)"},
    {"ISO-2022-JP-2", "Japanese (ISO-2022-JP-2)"},
    {"EUC-KR", "Korean (EUC-KR)"},
    {"CP949", "Korean (CP949)"},
    {"ISO-2022-KR", "Korean (ISO-2022-KR)"},
    {"LATIN6", "Nordic (LATIN6)"},
    {"LATIN4", "North European (LATIN4)"},
    {"KOI8-R", "Russian (KOI8-R)"},
    {"GBK", "Simplified Chinese (GBK)"},
    {"GB18030", "Simplified Chinese (GB18030)"},
    {"ISO-2022-CN-EXT", "Simplified Chinese (ISO-2022-CN-EXT)"},
    {"LATIN3", "South European (LATIN3)"},
    {"LATIN10", "South-Eastern European (LATIN10)"},
    {"TIS-620", "Thai (TIS-620)"},
    {"WINDOWS-874", "Thai (WINDOWS-874)"},
    {"EUC-TW", "Traditional Chinese (EUC-TW)"},
    {"BIG5", "Traditional Chinese (BIG5)"},
    {"BIG5-HKSCS", "Traditional Chinese (BIG5-HKSCS)"},
    {"LATIN5", "Turkish (LATIN5)"},
    {"WINDOWS-1254", "Turkish (WINDOWS-1254)"},
    {"KOI8-U", "Ukrainian (KOI8-U)"},
    {"WINDOWS-1258", "Vietnamese (WINDOWS-1258)"},
    {"VISCII", "Vietnamese (VISCII)"},
    {"LATIN1", "Western European (LATIN1)"},
    {"LATIN-9", "Western European (LATIN-9)"}
};


QString Path(QString dir, QString file)
{
    return dir + QDir::separator() + file;
}

QString EnsureDirExists(QString dir)
{
    QDir d(dir);
    if (!d.exists())
        d.mkpath(".");
    return d.absolutePath();
}

QString ConfigDir()
{
    return EnsureDirExists(Path(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation), APP_NAME));
}

QString DataDir()
{
    return EnsureDirExists(Path(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation), APP_NAME));
}

QString AppDataDir()
{
    QString dir(APP_DATA_DIR);
    if (dir[0] == '.')
        dir = Path(QCoreApplication::applicationDirPath(), dir);
    return QDir(dir).absolutePath();
}

QString SettingsPath()
{
    return Path(ConfigDir(), QString(APP_NAME) + ".ini");
}

QString TranslationsPath()
{
    QString dir(":/translations");
    if (QDir(dir).exists())
        return dir;
    return Path(AppDataDir(), "translations");
}

QString ScriptsPath()
{
    return Path(AppDataDir(), "scripts");
}

QList<QString> PluginsPaths()
{
    QList<QString> paths = {
        Path(ScriptsPath(), "plugins"),
        EnsureDirExists(Path(DataDir(), "plugins"))
    };
    return paths;
}

bool IsValidUrl(QString url)
{
    QRegExp rx("^[a-z]{2,}://", Qt::CaseInsensitive); // url
    return (rx.indexIn(url) != -1);
}

QString FormatTime(int _time, int _totalTime)
{
    QTime time = QTime::fromMSecsSinceStartOfDay(_time * 1000);
    if (_totalTime >= 3600) // hours
        return time.toString("h:mm:ss");
    if (_totalTime >= 60)   // minutes
        return time.toString("mm:ss");
    return time.toString("00:ss");   // seconds
}

QString FormatRelativeTime(int _time)
{
    QString prefix;
    if (_time < 0) {
        prefix = "-";
        _time = -_time;
    } else
        prefix = "+";
    QTime time = QTime::fromMSecsSinceStartOfDay(_time * 1000);
    if (_time >= 3600) // hours
        return prefix+time.toString("h:mm:ss");
    if (_time >= 60)   // minutes
        return prefix+time.toString("mm:ss");
    return prefix+time.toString("0:ss");   // seconds
}

QString FormatNumber(int val, int length)
{
    if (length < 10)
        return QString::number(val);
    else if (length < 100)
        return QString("%1").arg(val, 2, 10, QChar('0'));
    else
        return QString("%1").arg(val, 3, 10, QChar('0'));
}

QString FormatNumberWithAmpersand(int val, int length)
{
    if (length < 10)
        return "&"+QString::number(val);
    else if (length < 100) {
        if (val < 10)
            return "0&"+QString::number(val);
        return QString("%1").arg(val, 2, 10, QChar('0'));
    } else {
        if (val < 10)
            return "00&"+QString::number(val);
        return QString("%1").arg(val, 3, 10, QChar('0'));
    }
}

QString HumanSize(qint64 size)
{
    // taken from http://comments.gmane.org/gmane.comp.lib.qt.general/34914
    float num = size;
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while (num >= 1024.0 && i.hasNext()) {
        unit = i.next();
        num /= 1024.0;
    }
    return QString().setNum(num,'f',2)+" "+unit;
}

QString ShortenPathToParent(const Recent &recent)
{
    const int long_name = 100;
    if (recent.title != QString())
        return QString("%0 (%1)").arg(recent.title, recent.path);
    QString p = QDir::fromNativeSeparators(recent.path);
    int i = p.lastIndexOf('/');
    if (i != -1) {
        int j = p.lastIndexOf('/', i-1);
        if (j != -1) {
            QString parent = p.mid(j+1, i-j-1),
                    file = p.mid(i+1);
            // todo: smarter trimming
            if (parent.length() > long_name) {
                parent.truncate(long_name);
                parent += "..";
            }
            if (file.length() > long_name) {
                file.truncate(long_name);
                i = p.lastIndexOf('.');
                file += "..";
                if (i != -1) {
                    QString ext = p.mid(i);
                    file.truncate(file.length()-ext.length());
                    file += ext; // add the extension back
                }
            }
            return QDir::toNativeSeparators(parent+"/"+file);
        }
    }
    return QDir::toNativeSeparators(recent.path);
}

QStringList ToNativeSeparators(QStringList list)
{
    QStringList ret;
    for (auto element : list) {
        if (Util::IsValidLocation(element))
            ret.push_back(element);
        else
            ret.push_back(QDir::toNativeSeparators(element));
    }
    return ret;
}

QStringList FromNativeSeparators(QStringList list)
{
    QStringList ret;
    for (auto element : list)
        ret.push_back(QDir::fromNativeSeparators(element));
    return ret;
}

int GCD(int u, int v)
{
    int shift;
    if (u == 0) return v;
    if (v == 0) return u;
    for (shift = 0; ((u | v) & 1) == 0; ++shift) {
       u >>= 1;
       v >>= 1;
    }
    while ((u & 1) == 0)
        u >>= 1;
    do {
        while ((v & 1) == 0)
            v >>= 1;
        if (u > v) {
            unsigned int t = v;
            v = u;
            u = t;
        }
        v = v - u;
    } while (v != 0);
    return u << shift;
}

QString Ratio(int w, int h)
{
    int gcd = GCD(w, h);
    if (gcd == 0)
        return "0:0";
    return QString("%0:%1").arg(QString::number(w/gcd), QString::number(h/gcd));
}

QString GetLangName(QString code)
{
    return iso639CodeMap.value(code, "");
}

QString GetCharEncodingTitle(QString name)
{
    for (const auto &pair : charEncodingMap)
        if (pair.first == name)
            return pair.second;
    return "";
}

const QList<QPair<QString, QString> > &GetAllCharEncodings()
{
    return charEncodingMap;
}

}
