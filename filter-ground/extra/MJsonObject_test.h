#pragma once

#include <extra/MJsonObject.h>
#include <cassert>
#include <iostream>

inline void test_basic_parsing() {
    std::string json = R"({
        "name": "John",
        "age": 30
    })";

    MJsonObject obj = parse(json);
    assert(obj["name"].as_str() == "John");
    assert(obj["age"].as_int() == 30);
    std::cout << "test_basic_parsing passed" << std::endl;
}

inline void test_basic_types() {
    std::string json = R"({
        "string_val": "test",
        "string_special": "특수문자!@#$%^&*()_+\n\t\"\\",
        "string_empty": "",
        "int_val": 42,
        "int_max": 2147483647,
        "int_min": -2147483648,
        "int_zero": 0,
        "double_val": 3.14,
        "double_scientific": 1.23e-4,
        "double_large": 1.7976931348623157e+308,
        "double_small": 2.2250738585072014e-308,
        "double_precise": 3.141592653589793238462643383279,
        "double_negative": -123.456,
        "bool_val_true": true,
        "bool_val_false": false,
        "null_val": null
    })";

    MJsonObject obj = parse(json);

    // 문자열 테스트
    assert(obj["string_val"].as_str() == "test");
    assert(obj["string_special"].as_str() == "특수문자!@#$%^&*()_+\n\t\"\\");
    assert(obj["string_empty"].as_str() == "");

    // 정수 테스트
    assert(obj["int_val"].as_int() == 42);
    assert(obj["int_max"].as_int() == 2147483647);
    assert(obj["int_min"].as_int() == -2147483648);
    assert(obj["int_zero"].as_int() == 0);

    // 실수 테스트
    assert(std::abs(obj["double_val"].as_double() - 3.14) < 1e-10);
    assert(std::abs(obj["double_scientific"].as_double() - 1.23e-4) < 1e-10);
    assert(std::abs(obj["double_large"].as_double() - 1.7976931348623157e+308) < 1e-10);
    assert(std::abs(obj["double_small"].as_double() - 2.2250738585072014e-308) < 1e-10);
    assert(std::abs(obj["double_precise"].as_double() - 3.141592653589793238462643383279) < 1e-10);
    assert(std::abs(obj["double_negative"].as_double() - (-123.456)) < 1e-10);

    // 불리언 테스트
    assert(obj["bool_val_true"].as_bool() == true);
    assert(obj["bool_val_false"].as_bool() == false);

    // null 테스트
    assert(obj["null_val"].is_null());

    std::cout << "test basic types passed" << std::endl;
}

inline void test_nested_object() {
    std::string json = R"({
        "person": {
            "name": "John",
            "address": {
                "city": "Seoul",
                "country": "Korea"
            }
        }
    })";

    MJsonObject obj = parse(json);
    assert(obj["person"]["name"].as_str() == "John");
    assert(obj["person"]["address"]["city"].as_str() == "Seoul");
    assert(obj["person"]["address"]["country"].as_str() == "Korea");

    std::cout << "test_nested_object passed" << std::endl;
}

inline void test_nested_array() {
    std::string json = R"({
        "numbers": [1, 2, 3],
        "matrix": [[1, 2], [3, 4]],
        "objects": [
            {"id": 1, "value": "first"},
            {"id": 2, "value": "second"}
        ]
    })";

    MJsonObject obj = parse(json);
    assert(obj["numbers"][0].as_int() == 1);
    assert(obj["matrix"][1][0].as_int() == 3);
    assert(obj["objects"][1]["value"].as_str() == "second");

    std::cout << "test_nested_array passed" << std::endl;
}

inline void test_error_cases() {
    try {
        MJsonObject obj = parse("invalid json");
        assert(false); // Should not reach here
    } catch (const std::exception&) {
        // Expected exception
    }

    MJsonObject obj = parse("{}");
    try {
        obj["non_existent"].as_str();
        assert(false); // Should not reach here
    } catch (const std::exception&) {
        // Expected exception
    }

    std::cout << "test_error_cases passed" << std::endl;
}

inline void test_as_conversions() {
    std::string json = R"({
        "number": "42",
        "array": [1, 2, 3],
        "object": {"key": "value"}
    })";

    MJsonObject obj = parse(json);
    assert(obj["number"].as_str() == "42");
    assert(obj["array"].is_array());
    assert(obj["object"].is_object());

    std::cout << "test_as_conversions passed" << std::endl;
}

inline void test_as_or_conversions() {
    std::string json = R"({
        "exists": 42
    })";

    MJsonObject obj = parse(json);
    assert(obj["exists"].as_int_or(0) == 42);
    assert(obj["non_existent"].as_int_or(0) == 0);
    assert(obj["non_existent"].as_str_or("default") == "default");
}

inline void test_required() {
    std::string json = R"({
        "user": {
            "name": "John",
            "age": 30
        }
    })";

    MJsonObject obj = parse(json);
    obj.required("user", [](const MJsonObject& user) {
        assert(user["name"].as_str() == "John");
        assert(user["age"].as_int() == 30);
    });

    std::cout << "test_required passed" << std::endl;
}

inline void test_required_for() {
    std::string json = R"({
        "users": [
            {"name": "John", "age": 30},
            {"name": "Jane", "age": 25}
        ]
    })";

    MJsonObject obj = parse(json);
    obj.required_for("users", [](const MJsonArray& users, size_t index) {
        if (index == 0) {
            assert(users[index]["name"].as_str() == "John");
            assert(users[index]["age"].as_int() == 30);
        } else {
            assert(users[index]["name"].as_str() == "Jane");
            assert(users[index]["age"].as_int() == 25);
        }
    });

    std::cout << "test_required_for passed" << std::endl;
}

inline void test_optional() {
    std::string json = R"({
        "exists": "value"
    })";

    MJsonObject obj = parse(json);

    bool exists_called = false;
    bool not_exists_called = false;

    obj.optional("exists", [&](const MJsonValue& val) {
        exists_called = true;
        assert(val.as_str() == "value");
    });

    obj.optional("not_exists", [&](const MJsonValue&) {
        assert(false); // Should not be called
    }, [&]() {
        not_exists_called = true;
    });

    assert(exists_called);
    assert(not_exists_called);

    std::cout << "test_optional passed" << std::endl;
}

inline void test_optional_for() {
    std::string json = R"({
        "items": [
            {"id": 1},
            {"id": 2}
        ]
    })";

    MJsonObject obj = parse(json);

    bool items_called = false;
    obj.optional_for("items", [&](const MJsonArray& items, size_t index) {
        items_called = true;
        assert(items[index]["id"].as_int() == index + 1);
    });

    bool no_items_called = false;
    obj.optional_for("no_items", [&](const MJsonArray&, size_t) {
        assert(false); // Should not be called
    }, [&]() {
        no_items_called = true;
    });

    assert(items_called);
    assert(no_items_called);

    std::cout << "test_optional_for passed" << std::endl;
}

inline void test_move_member_function() {
    std::string json = R"({
        "source": {"value": 42}
    })";

    MJsonObject obj = parse(json);

    // 이동 전 원본 값 확인
    assert(obj["source"]["value"].as_int() == 42);

    // 값 이동
    MJsonObject moved = obj["source"].take<MJsonObject>();

    // 이동된 값 확인
    assert(moved["value"].as_int() == 42);

    // 원본이 비워졌는지 확인
    try {
        obj["source"]["value"].as_int();
        assert(false); // 여기에 도달하면 안됨
    } catch (const std::exception&) {
        // 예상된 예외
    }

    std::cout << "test_move_member_function passed" << std::endl;
}

inline void test_complex_mixed_access_and_functional() {
    std::string json = R"({
        "company": {
            "departments": [
                {
                    "name": "Engineering",
                    "employees": [
                        {"name": "John", "skills": ["C++", "Python"]},
                        {"name": "Jane", "skills": ["Java", "JavaScript"]}
                    ]
                },
                {
                    "name": "Sales",
                    "employees": [
                        {"name": "Bob", "skills": ["Negotiation"]}
                    ]
                }
            ]
        }
    })";

    MJsonObject obj = parse(json);

    obj.required("company", [](const MJsonObject& company) {
        company.required_for("departments", [](const MJsonArray& departments, size_t index) {
            if (index == 0) {
                assert(departments[index]["name"].as_str() == "Engineering");
                departments[index].required_for("employees", [](const MJsonArray& employees, size_t emp_index) {
                    if (emp_index == 0) {
                        assert(employees[emp_index]["name"].as_str() == "John");
                        assert(employees[emp_index]["skills"][0].as_str() == "C++");
                    }
                });
            }
        });
    });

    std::cout << "test_complex_mixed_access_and_functional passed" << std::endl;
}

inline void test_extremely_complex_mixed_access() {
    std::string json = R"({
        "config": {
            "app": {
                "name": "MyApp",
                "version": "1.0.0",
                "settings": {
                    "debug": true,
                    "cache": {
                        "enabled": true,
                        "max_size": 1024
                    }
                }
            },
            "database": {
                "main": {
                    "host": "localhost",
                    "port": 3306,
                    "credentials": {
                        "user": "admin",
                        "password": "secret"
                    },
                    "options": {
                        "pool": {
                            "min": 5,
                            "max": 10
                        },
                        "timeout": 30,
                        "retry": {
                            "attempts": 3,
                            "delay": 1000
                        }
                    }
                },
                "replica": {
                    "hosts": [
                        {"host": "replica1", "port": 3306},
                        {"host": "replica2", "port": 3306}
                    ],
                    "options": {
                        "read_only": true,
                        "load_balancing": "round-robin"
                    }
                }
            },
            "logging": {
                "level": "info",
                "outputs": [
                    {"type": "console", "format": "json"},
                    {"type": "file", "path": "/var/log/app.log"}
                ],
                "options": {
                    "rotation": {
                        "enabled": true,
                        "max_files": 5,
                        "max_size": "100M"
                    }
                }
            }
        }
    })";

    MJsonObject obj = parse(json);

    // 체이닝과 optional/required를 혼합하여 테스트
    obj["config"]["database"].required("main", [](const MJsonObject& main) {
        assert(main["host"].as_str() == "localhost");

        // 체이닝 후 optional 사용
        main["credentials"].optional("user", [](const MJsonValue& user) {
            assert(user.as_str() == "admin");
        });

        // 체이닝 후 required 사용
        main.required("options", [](const MJsonObject& options) {
            // 더 깊은 체이닝과 optional 혼합
            options["pool"].optional("min", [](const MJsonValue& min) {
                assert(min.as_int() == 5);
            });

            // optional 내부에서 체이닝
            options.optional("retry", [](const MJsonValue& retry) {
                assert(retry["attempts"].as_int() == 3);
                assert(retry["delay"].as_int() == 1000);
            });
        });
    });

    // 체이닝 후 optional_for 사용
    obj["config"]["database"]["replica"].optional_for("hosts", [](const MJsonArray& hosts, size_t index) {
        if (index == 0) {
            assert(hosts[index]["host"].as_str() == "replica1");
        }
    });

    // 체이닝 후 required_for와 optional 혼합
    obj["config"]["logging"].required_for("outputs", [](const MJsonArray& outputs, size_t index) {
        if (index == 0) {
            assert(outputs[index]["type"].as_str() == "console");
        }
    });

    // optional 내부에서 체이닝
    obj["config"]["logging"].optional("options", [](const MJsonValue& options) {
        assert(options["rotation"]["enabled"].as_bool() == true);
    });

    std::cout << "test_extremely_complex_mixed_access passed" << std::endl;
}

inline void test_extremely_complex_mixed_access_with_index() {
    std::string json = R"({
        "data": {
            "items": [
                {
                    "id": 1,
                    "name": "Item 1",
                    "details": {
                        "categories": ["A", "B", "C"],
                        "specs": [
                            {"key": "size", "value": "large"},
                            {"key": "color", "value": "blue"}
                        ]
                    },
                    "variants": [
                        {
                            "code": "V1",
                            "stock": 100,
                            "prices": [
                                {"region": "US", "amount": 99.99},
                                {"region": "EU", "amount": 89.99}
                            ],
                            "features": {
                                "main": ["feature1", "feature2"],
                                "optional": ["opt1", "opt2"]
                            }
                        }
                    ]
                }
            ],
            "metadata": {
                "total": 1,
                "filters": ["active", "in-stock"]
            }
        }
    })";

    MJsonObject obj = parse(json);

    // 인덱스 기반 접근과 문자열 기반 접근을 혼합하여 테스트
    obj.required("data", [](const MJsonObject& data) {
        // 배열에 대한 인덱스 기반 required
        data["items"].required(0, [](const MJsonValue& item) {
            assert(item["id"].as_int() == 1);

            // 배열 내부의 객체에 대한 인덱스 기반 required_for
            item["details"]["categories"].required_for(0, [](const MJsonArray& categories, size_t cat_index) {
                if (cat_index == 0) {
                    assert(categories[cat_index].as_str() == "A");
                }
            });

            // 중첩된 배열에 대한 인덱스 기반 optional
            item["details"]["specs"].optional(1, [](const MJsonValue& spec) {
                assert(spec["key"].as_str() == "color");
            });

            // variants 배열에 대한 인덱스 기반 required
            item["variants"].required(0, [](const MJsonValue& variant) {
                // prices 배열에 대한 인덱스 기반 required
                variant["prices"].required(0, [](const MJsonValue& price) {
                    assert(price["region"].as_str() == "US");
                });

                // features에 대한 인덱스 기반 optional_for
                variant["features"]["main"].optional_for(0, [](const MJsonArray& features, size_t feat_index) {
                    assert(features[feat_index].as_str() == "feature1");
                });
            });
        });

        // metadata에 대한 인덱스 기반 optional
        data["metadata"]["filters"].optional(1, [](const MJsonValue& filter) {
            assert(filter.as_str() == "in-stock");
        });
    });

    std::cout << "test_extremely_complex_mixed_access_with_index passed" << std::endl;
}

inline void
test_last()
{

  const std::string config_json = R"(
  {
    "config": {
      "database": {
        "host": "127.0.0.1",
        "port": 3306,
        "username": "...",
        "password": "...",
        "options": {
          "timeout": 30,
          "pool": 10
        }
      }
    }
  })";

  MJsonObject obj = parse(config_json);

  // 필수 설정들은 간단한 체이닝으로 처리, 모두 required 임.
  std::cout << "CONFIG host " << obj["config"]["database"]["host"].as_str() << std::endl;
  std::cout << "CONFIG port " << obj["config"]["database"]["port"].as_int() << std::endl;

  // 선택적인 설정은 콜백으로 처리
  obj["config"]["database"].optional("options", [&](const MJsonObject &options)
  {
    // 여기서는 여러 설정을 한번에 다룰 수 있어요
    std::cout << "CONFIG timeout " << options["timeout"].as_int_or(30) << std::endl;
    std::cout << "CONFIG pool    " << options["pool"].as_int_or(10) << std::endl;
    std::cout << "CONFIG ssl     " << (options["ssl"].as_bool_or(false) ? "true" : "false") << std::endl;
  },
  [&]()
  { // default 처리
    std::cout << "CONFIG timeout 300   " << std::endl;
    std::cout << "CONFIG pool    0     " << std::endl;
    std::cout << "CONFIG ssl     false " << std::endl;
  });

  // 또는
  obj.required("config", [&](const MJsonObject &config)
  {
    config.required("database", [&](const MJsonObject &database)
    {
      database.optional("options", [&](const MJsonObject &options)
      {
        // 여기서는 여러 설정을 한번에 다룰 수 있어요
        std::cout << "CONFIG timeout " << options["timeout"].as_int_or(30) << std::endl;
        std::cout << "CONFIG pool    " << options["pool"].as_int_or(10) << std::endl;
        std::cout << "CONFIG ssl     " << (options["ssl"].as_bool_or(false) ? "true" : "false") << std::endl;
      },
      [&]()
      { // default 처리
        std::cout << "not found, default" << std::endl;
        std::cout << "CONFIG timeout 300   " << std::endl;
        std::cout << "CONFIG pool    0     " << std::endl;
        std::cout << "CONFIG ssl     false " << std::endl;
      });
    });
  });

  const std::string json = R"({
      "number": 42,
      "string": "test",
      "bool": true,
      "null": null,
      "array": [1, 2, 3],
      "object": {
          "key": "value"
      },
      "depth1" : {
        "depth2" : {
          "depth3" : {
             "name": "value3"
           }
        }
      },
      "arrays": [[1,2,3], [4,5,6]]
  })";

  MJsonObject doc = parse(json);

  std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << doc["object"]["key"].as_str() << std::endl;
  std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << doc["object"]["noname"].as_str_or("unknown") << std::endl;

  doc.required("depth1", [&](const MJsonObject &depth1)
  {
    depth1.required("depth2", [&](const MJsonObject &depth2)
    {
      depth2.required("depth3", [&](const MJsonObject &depth3)
      {
        std::cout << depth3["name"].as_str() << std::endl;
      });
    });
  });

  try
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << doc["object"]["key"].as_int() << std::endl;
  }
  catch (const std::runtime_error &e)
  {
    // /object/key: expected number, got string
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << e.what() << std::endl;
  }

  try
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << doc["object"]["key"]["abc"].as_str() << std::endl;
  }
  catch (const std::runtime_error &e)
  {
    // /object/key: expected MJsonObject, got MJsonValue
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << e.what() << std::endl;
  }

  doc.required("object", [&](const MJsonObject &object)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << object["key"].as_str() << std::endl;
  });


  doc.required("number", [&](const MJsonValue &value)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << value.as_int() << std::endl;;
  });

  doc.required_for("array", [&](const MJsonArray &arr, size_t index)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << index << ":" << arr[index].as_int() << std::endl;
  });

  doc["array"].required(0, [&](const MJsonValue &val)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << val.as_int() << std::endl;
  });

  doc["arrays"].required_for(0, [&](const MJsonArray &val, size_t index)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << val[index].as_int() << std::endl;
  });

  doc.optional("noname", [&](const MJsonValue &obj)
  {
    std::cout << obj.as_str() << std::endl;
  }, [&]{ std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << "not found noname" << std::endl; });

  doc.optional("noname", [&](const MJsonValue &obj)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << obj.as_str() << std::endl;
  });

  doc.optional_for("noname_arr", [&](const MJsonArray &value, size_t index)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << value[index].as_int() << std::endl;
  },
  [&]
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << "not found noname_arr" << std::endl;
  });

  doc.optional_for("noname_arr", [&](const MJsonArray &value, size_t index)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << value[index].as_int() << std::endl;
  });


  doc["arrays"].optional_for(2, [&](const MJsonArray &val, size_t index)
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << val[index].as_int() << std::endl;
  },
  [&]()
  {
    std::cout << __LINE__ << ":" << __FUNCTION__ << ":" << ":" << "not found 2 of arrays" << std::endl; }
  );

}

inline void test_all()
{
    test_basic_parsing();
    test_basic_types();
    test_nested_object();
    test_nested_array();
    test_error_cases();
    test_as_conversions();
    test_as_or_conversions();
    test_required();
    test_required_for();
    test_optional();
    test_optional_for();
    test_move_member_function();
    test_complex_mixed_access_and_functional();
    test_extremely_complex_mixed_access();
    test_extremely_complex_mixed_access_with_index();
    test_last();

    std::cout << "All tests passed" << std::endl;
}
