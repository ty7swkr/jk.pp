# MJsonObject 사용 가이드

### 개요
#### rapidjson 사용.
```cpp
if (doc.HasMember("users") &&
    doc["users"].IsArray() &&
    doc["users"].Size() > 0 &&
    doc["users"][0].HasMember("contact") &&
    doc["users"][0]["contact"].HasMember("email") &&
    doc["users"][0]["contact"]["email"].IsString())
{
  std::string email = doc["users"][0]["contact"]["email"].GetString();
  return true;
}
else
{
  return false;
}
```

#### MJsonObject 사용
```cpp
try
{
  std::string email = doc["users"][0]["contact"]["email"].as_str_or("no-email");
  return true;
}
catch (const std::runtime_error &e)
{
  std::cout << e.what() << std::endl;
  return false;
}
```

### 클래스 구성

읽기 전용이며 아래와 같이 구성되어 있습니다.
- `MJsonValue`: 모든 JSON 값의 기본 클래스입니다. 문자열, 숫자, 불리언, null 등의 기본 타입 클래스
- `MJsonArray`: JSON 배열 처리 클래스
- `MJsonObject`: JSON 객체(키-값 쌍)를 위한 클래스

### 소스 구성
- MJsonObject.h: 클래스 선언
- MJsonObject.inl: MJsonObject 구현체
- MJsonValue.inl: MJsonValue 구현체
- MJsonArray.inl: MJsonArray 구현체


### JSON 문자열 파싱

가장 기본적인 JSON 파싱은 `parse()` 함수를 사용합니다:

```cpp
const char* json = R"({
    "name": "홍길동",
    "age": 30,
    "hobbies": ["독서", "운동"]
})";

MJsonObject obj = parse(json);
```

### 예외 처리

반드시 예외 처리가 필요합니다. std::runtime_error를 throw합니다.

```cpp
try {
    MJsonObject config = parse(json_str);
    
    // 필수 값, json에 server or port가 없으면 throw
    int port = config["system"][0]["server"]["port"].as_int();
    
} catch (const std::runtime_error &e) {
    // 오류 처리
}
```

## 접근 방식
- 체이닝([])을 이용한 접근
- 함수형 방식 : required, required_for(배열), optional, optional_for(배열)
- 변수 접근 : as_*(as_int(), as_str()...), as_*_or(as_int_or(default_value))
- 멤버를 이용한 iterator: MJsonObject::members(name, value), MJsonArray::elements
- 객체 참조와 복사, 이동 
- 언제 throw 되는가
- 기타함수: has(?) 와 size() 

### 체이닝 접근: []
체이닝은 항상 required로 동작합니다.

```cpp
try
{
    std::string name = obj["user"]["name"].as_string();
    int age   = obj["user"]["age"].as_int();
    
    // json 배열
    // "wallet": [ "paper": 2000, "coin": 500 ]
    int cache = obj["wallet"][0]["paper"].as_int();
    int coin = obj["wallet"][1]["coin"].as_int();
    
    // "number": [ 1, 2, 3, 4 ,5 ]
    int number0 = obj["number"][0].as_int();
    
    // 배열에 없는 원소 접근 throw 안하고 10 리턴.
    int number9 = obj["number"][9].as_int_or(10);
}
catch (const std::runtime_error &e)
{
    // ex) "/user/name: not found"
    std::cerr << e.what() << std::endl;
}
```

### 함수형 방식 : required, required_for(배열), optional, optional_for(배열)

JSON 구조를 다룰 때 required, required_for, optional, optional_for 사용하면 
JSON 계층구조와 동일하게 접근할 수 있습니다.

```cpp
const char* json = R"({
    "player": {
        "name": "전사1",
        "items": [
            {"name": "롱소드", "damage": 50},
            {"name": "방패" , "defense": 30}
        ],
        "options": {
            "thing": "가방"
        }
    }
})";

MJsonObject doc = parse(json);

// player 객체 접근
doc.required("player", [&](const MJsonObject& player)
{
    // 기본 정보 처리
    std::string name = player["name"].as_string();

    // 아이템 배열 처리
    player.required_for("items", [&](const MJsonArray& items, size_t index)
    {
        std::string item_name = items[index]["name"].as_string();
        int damage            = items[index]["damage"].as_int_or(0);
        int defense           = items[index]["defense"].as_int_or(0);
    });

    // 요긴 optional
    player.optional("option", [&](const MJsonObject &option) 
    {
        // 요긴 자동 required
        std::string thing = option["thing"]; 
    });
});
```

### 체이닝([]) & 함수형 혼합
```
doc["game"]["player"].optional("option", [&](const MJsonObject& option)
{
    std::string thing = option["thing"];
});
```

### 변수 접근
```
// exist의 값을 int로 리턴. exist가 없으면 throw발생
int exist = system["exist"].as_int();

// nonexist가 없는 경우 인자로 받은 9를 리턴. throw발생 하지 않음..
int nonexist = system["nonexist"].as_int_or(9);

// "system"이 없는 경우 throw발생, nonexist가 throw없이 인자로 받은 3 리턴.
// as_*_or 함수는 직전의 필드에 대해서만 영향도를 가짐.
int x = root["system"]["nonexist].as_int_or(3);
```

### 멤버를 이용한 iterator
```
MJsonObject obj["system"];
for (auto &it : obj.members)
{
    std::string name = it->first;
    // const MJsonObject & 또는 cosnt MJsonArray & 또는 const MJsonValue &: json의 구조에 따름.
    auto object = it->second;
}

const MJsonArray &arr = obj["system"]["arr"];
for (auto &it : arr.elements)
{
    // const MJsonObject & 또는 cosnt MJsonArray & 또는 const MJsonValue &: json의 구조에 따름.
    auto &item = *it;
}
```

### 객체 참조와 복사 & 이동

MJsonObject, MJsonArray, MJsonValue는 객체의 참조와 복사에 있어 다음과 같은 특징이 있습니다:

#### 참조 접근
복사 비용이 발생하지 않습니다.

```cpp
const MJsonValue& value = obj["user"];  
```

#### 복사 대입
참조 없이 접근하는 경우 복사(deep copy)를 수행합니다:

```cpp
MJsonObject copy_obj = obj["config"];  
```

#### 객체 이동: take() 함수

`take()` 함수는 객체의 소유권을 이전할 때 사용합니다. 원본 객체의 내용은 이동되어 비워집니다:

```cpp
const char* json = R"({
    "user": {
        "name": "홍길동",
        "items": ["지갑", "열쇠"]
    }
})";

MJsonObject doc = parse(json);

// user 객체의 소유권을 이전
MJsonObject user = doc["user"].take<MJsonObject>();

// 이제 원본의 user는 비어있고, 새로운 user 객체가 내용을 가짐
try {
    doc["user"]["name"].as_string();  // 예외 발생
} catch (const std::runtime_error& e) {
    // "user" 필드가 비어있어 예외 발생
}

// 이전된 객체는 정상적으로 사용 가능
std::string name = user["name"].as_string();  // "홍길동"
```

take() 함수의 주요 속성:
- 객체의 내용을 새로운 객체로 이동시킵니다
- 원본 객체는 비어있게 됩니다
- 이동된 객체는 독립적으로 사용할 수 있습니다

이러한 방식으로 객체를 다룰 때는 다음 사항을 고려해야 합니다:
- 참조로 접근할 때는 원본 객체의 수명을 고려해야 합니다
- 복사가 필요한 경우 명시적으로 복사 생성이나 대입을 사용합니다
- 소유권 이전이 필요한 경우 take() 함수를 활용합니다


### 언제 throw 되는가

반드시 예외 처리가 필요하며 "지연 평가(lazy evaluation)" 방식을 사용하고 있어, 실제로 값을 사용하는 시점에 예외가 발생합니다:

```cpp
MJsonObject obj = parse(json);

// 이때는 예외를 발생시키지 않습니다
obj["존재하지않는필드"];               

// 이 시점에서 예외가 발생합니다. as_int()등 사용시점.
obj["존재하지않는필드"].as_int();      

// 예외 발생 없이 기본값을 반환합니다.
// as_int_or는 필드가 존재하지 않는 경우 인자값(9)을 리턴
obj["존재하지않는필드"].as_int_or(9);  

// "존재하지않는필드1"가 없으면 예외 발생.
// "존재하지않는필드2"가 없어도 예외 발생 안함.
// as_*_or 함수는 바로 직전의 필드에 대해서만 영향도를 가짐.
obj["존재하지않는필드1"]["존재하지않는필드2"].as_int_or(3);
```

### 파일 처리

JSON 파일을 직접 읽어 파싱할 수 있습니다:

```cpp
// 파일에서 JSON을 읽고 파싱합니다
MJsonObject obj = parse_file("config.json");

// 파일 정상적으로 읽게 되면 파싱전 json내용을 람다 함수로 전달합니다.
// 람다함수가 없는 경우 전달하지 않습니다.
MJsonObject obj = parse_file("config.json", [](const std::string& content) {
    std::cout << content << std::endl; 
});
```

### 기타함수
- optional이 귀찮을때: has(?)
  - user가 json-오브젝트
    - {"user": {"name": "홍길동", "skill", "나르기"}} 
    - doc["user"].has("name") == true<br>

  - box가 json-배열
    - {"box": [1, 2, 3] }
    - doc["box"].has(0) == true<br>

  - box가 json-배열-오브젝트
     - {"box": [{"card": "samsung"}, {"cache": 1000}]}
     - doc["box"][0].has("card") == true<br>

- 멤버개수를 알고 싶을때: size()
  - {"user": {"name":"홍길동", "skill", "나르기"}} }
    - doc["user"].size() == 2<br>

  - {"box", [1,2,3]}
    - doc["box"].size() == 3

<br><br><br>
